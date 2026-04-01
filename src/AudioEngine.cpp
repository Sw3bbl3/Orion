#include "orion/AudioEngine.h"
#include "orion/InstrumentTrack.h"
#include "orion/HostContext.h"
#include "orion/SettingsManager.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>
#include <chrono>
#include <mutex>
#include <vector>
#include <thread>
#include <map>
#include <set>
#include <functional>
#include <queue>
#include <future>
#include <array>
#include <filesystem>
#include <sstream>
#include <cstdlib>
#include "orion/ThreadPool.h"
#include "orion/CompressorNode.h"
#include <juce_audio_formats/juce_audio_formats.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Orion {

    std::atomic<int> gDiskActivityCounter{0};

    thread_local HostContext gHostContext;


    static std::vector<float> metronomeLUT;
    static constexpr size_t METRONOME_LUT_SIZE = 4096;
    static constexpr size_t METRONOME_LUT_MASK = METRONOME_LUT_SIZE - 1;
    static std::once_flag lutInitFlag;

    static void ensureMetronomeLUT() {
        std::call_once(lutInitFlag, []() {
            metronomeLUT.resize(METRONOME_LUT_SIZE);
            for (size_t i = 0; i < METRONOME_LUT_SIZE; ++i) {
                metronomeLUT[i] = static_cast<float>(std::sin(2.0 * M_PI * (double)i / (double)METRONOME_LUT_SIZE));
            }
        });
    }

    static bool transcodeWithLame(const juce::File& wavInput,
                                  const juce::File& mp3Output,
                                  int bitrateKbps,
                                  std::string& err)
    {
#ifndef ORION_LAME_EXECUTABLE
        juce::ignoreUnused(wavInput, mp3Output, bitrateKbps);
        err = "MP3 export unavailable: ORION_LAME_EXECUTABLE is not configured.";
        return false;
#else
        auto quoteForShell = [](const juce::String& value) {
            return "\"" + value.replace("\"", "\\\"") + "\"";
        };
        juce::File lameExe(juce::String(ORION_LAME_EXECUTABLE));
        if (!lameExe.existsAsFile()) {
            err = "MP3 export unavailable: configured LAME executable was not found.";
            return false;
        }

        juce::String cmd;
        cmd << quoteForShell(lameExe.getFullPathName()) << " --silent --noreplaygain --cbr -b "
            << juce::String(std::clamp(bitrateKbps, 8, 320))
            << " " << quoteForShell(wavInput.getFullPathName())
            << " " << quoteForShell(mp3Output.getFullPathName());

        auto code = std::system(cmd.toRawUTF8());
        if (code != 0) {
            err = "LAME encoder failed with exit code " + std::to_string(code) + ".";
            return false;
        }
        return mp3Output.existsAsFile() && mp3Output.getSize() > 0;
#endif
    }

    AudioEngine::AudioEngine()
        : sampleRate(44100.0),
          blockSize(128),
          currentFrameIndex(0),
          state(TransportState::Stopped),
          looping(false),
          loopStart(0),
          loopEnd(0),
          commandQueue(256),
          releaseQueue(64)
    {
        ensureMetronomeLUT();

        currentProcessingGraph = std::make_shared<ProcessingGraph>();

        scopeBuffer.resize(2048, 0.0f);


        masterNode = std::make_shared<MasterNode>();
        masterNode->setFormat(2, blockSize);
        masterNode->setSampleRate(sampleRate);

        monitoringNode = std::make_shared<EnvironmentNode>();
        monitoringNode->setFormat(2, blockSize);
        monitoringNode->setSampleRate(sampleRate);
    }

    void AudioEngine::setSampleRate(double rate) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        sampleRate = rate;

        AudioGraphEvent event;
        event.type = AudioGraphEvent::SetSampleRate;
        event.sampleRate = rate;
        sendEvent(event);
    }

    void AudioEngine::setBlockSize(size_t size) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        blockSize = size;

        AudioGraphEvent event;
        event.type = AudioGraphEvent::SetBlockSize;
        event.blockSize = size;
        sendEvent(event);
    }

    void AudioEngine::play() {
        if (state.load() == TransportState::Stopped || state.load() == TransportState::Paused) {
            playStartPosition = currentFrameIndex.load();
        }
        state = TransportState::Playing;
    }

    void AudioEngine::pause() {
        state = TransportState::Paused;
        if (SettingsManager::get().returnToStartOnStop) {
            currentFrameIndex = playStartPosition.load();
        }
        AudioGraphEvent event;
        event.type = AudioGraphEvent::AllNotesOff;
        sendEvent(event);
    }

    void AudioEngine::stop() {
        state = TransportState::Stopped;
        if (SettingsManager::get().returnToStartOnStop) {
            currentFrameIndex = playStartPosition.load();
        } else {
            currentFrameIndex = 0;
        }
        AudioGraphEvent event;
        event.type = AudioGraphEvent::AllNotesOff;
        sendEvent(event);
    }

    void AudioEngine::panic() {
        AudioGraphEvent event;
        event.type = AudioGraphEvent::AllNotesOff;
        sendEvent(event);
    }

    void AudioEngine::record() {
        if (state.load() == TransportState::Stopped || state.load() == TransportState::Paused) {
            playStartPosition = currentFrameIndex.load();
        }
        state = TransportState::Recording;
    }

    void AudioEngine::setCursor(uint64_t frame) {
        currentFrameIndex = frame;
        playStartPosition = frame;
        AudioGraphEvent event;
        event.type = AudioGraphEvent::AllNotesOff;
        sendEvent(event);
    }

    void AudioEngine::addTempoMarker(uint64_t position, float newBpm) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        tempoMarkers.push_back({position, newBpm});
        std::sort(tempoMarkers.begin(), tempoMarkers.end(), [](const TempoMarker& a, const TempoMarker& b) {
            return a.position < b.position;
        });


        AudioGraphEvent clearEvent;
        clearEvent.type = AudioGraphEvent::ClearTempoMarkers;
        sendEvent(clearEvent);

        for (const auto& m : tempoMarkers) {
            AudioGraphEvent addEvent;
            addEvent.type = AudioGraphEvent::AddTempoMarker;
            addEvent.position = m.position;
            addEvent.bpm = m.bpm;
            sendEvent(addEvent);
        }
    }

    void AudioEngine::removeTempoMarker(int index) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (index >= 0 && index < (int)tempoMarkers.size()) {
            tempoMarkers.erase(tempoMarkers.begin() + index);


            AudioGraphEvent clearEvent;
            clearEvent.type = AudioGraphEvent::ClearTempoMarkers;
            sendEvent(clearEvent);

            for (const auto& m : tempoMarkers) {
                AudioGraphEvent addEvent;
                addEvent.type = AudioGraphEvent::AddTempoMarker;
                addEvent.position = m.position;
                addEvent.bpm = m.bpm;
                sendEvent(addEvent);
            }
        }
    }

    void AudioEngine::clearMarkers() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        tempoMarkers.clear();
        timeSigMarkers.clear();
        regionMarkers.clear();

        AudioGraphEvent event;
        event.type = AudioGraphEvent::ClearTempoMarkers;
        sendEvent(event);
    }

    const std::vector<AudioEngine::TempoMarker>& AudioEngine::getTempoMarkers() const {
        return tempoMarkers;
    }

    void AudioEngine::addTimeSigMarker(uint64_t position, int num, int den) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        timeSigMarkers.push_back({position, num, den});
        std::sort(timeSigMarkers.begin(), timeSigMarkers.end(), [](const TimeSigMarker& a, const TimeSigMarker& b) {
            return a.position < b.position;
        });
    }

    void AudioEngine::removeTimeSigMarker(int index) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (index >= 0 && index < (int)timeSigMarkers.size()) {
            timeSigMarkers.erase(timeSigMarkers.begin() + index);
        }
    }

    const std::vector<AudioEngine::TimeSigMarker>& AudioEngine::getTimeSigMarkers() const {
        return timeSigMarkers;
    }

    void AudioEngine::addRegionMarker(uint64_t position, const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        regionMarkers.push_back({position, name});
        std::sort(regionMarkers.begin(), regionMarkers.end(), [](const RegionMarker& a, const RegionMarker& b) {
            return a.position < b.position;
        });
    }

    void AudioEngine::removeRegionMarker(int index) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (index >= 0 && index < (int)regionMarkers.size()) {
            regionMarkers.erase(regionMarkers.begin() + index);
        }
    }

    void AudioEngine::updateRegionMarker(int index, uint64_t position, const std::string& name) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (index >= 0 && index < (int)regionMarkers.size()) {
            regionMarkers[index].position = position;
            regionMarkers[index].name = name;
            std::sort(regionMarkers.begin(), regionMarkers.end(), [](const RegionMarker& a, const RegionMarker& b) {
                return a.position < b.position;
            });
        }
    }

    const std::vector<AudioEngine::RegionMarker>& AudioEngine::getRegionMarkers() const {
        return regionMarkers;
    }

    std::shared_ptr<AudioTrack> AudioEngine::createTrack() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        auto track = std::make_shared<AudioTrack>();
        track->setFormat(2, blockSize);
        track->setSampleRate(sampleRate);
        track->setName("Audio " + std::to_string(tracks.size() + 1));

        tracks.push_back(track);

        AudioGraphEvent event;
        event.type = AudioGraphEvent::TrackAdded;
        event.track = track;
        sendEvent(event);

        rebuildGraph();
        return track;
    }

    std::shared_ptr<AudioTrack> AudioEngine::createAuxTrack() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        auto track = std::make_shared<AudioTrack>();
        track->setFormat(2, blockSize);
        track->setSampleRate(sampleRate);
        track->setName("Aux " + std::to_string(tracks.size() + 1));

        track->setColor(0.3f, 0.3f, 0.8f);
        track->setSoloSafe(true);

        tracks.push_back(track);

        AudioGraphEvent event;
        event.type = AudioGraphEvent::TrackAdded;
        event.track = track;
        sendEvent(event);

        return track;
    }

    std::shared_ptr<AudioTrack> AudioEngine::createBusTrack() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        auto track = std::make_shared<AudioTrack>();
        track->setFormat(2, blockSize);
        track->setSampleRate(sampleRate);
        track->setName("Bus " + std::to_string(tracks.size() + 1));
        track->setColor(0.6f, 0.2f, 0.6f);
        track->setSoloSafe(true);

        tracks.push_back(track);

        AudioGraphEvent event;
        event.type = AudioGraphEvent::TrackAdded;
        event.track = track;
        sendEvent(event);

        return track;
    }

    std::shared_ptr<InstrumentTrack> AudioEngine::createInstrumentTrack() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        auto track = std::make_shared<InstrumentTrack>();
        track->setFormat(2, blockSize);
        track->setSampleRate(sampleRate);
        track->setName("Inst " + std::to_string(tracks.size() + 1));

        tracks.push_back(track);

        AudioGraphEvent event;
        event.type = AudioGraphEvent::TrackAdded;
        event.track = track;
        sendEvent(event);

        return track;
    }

    void AudioEngine::addTrack(std::shared_ptr<AudioTrack> track) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        tracks.push_back(track);

        AudioGraphEvent event;
        event.type = AudioGraphEvent::TrackAdded;
        event.track = track;
        sendEvent(event);

        rebuildGraph();
    }

    void AudioEngine::removeTrack(std::shared_ptr<AudioTrack> track) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (!track) return;

        AudioNode* removedNode = track.get();

        for (auto& existing : tracks) {
            if (!existing) continue;

            if (existing != track && existing->getOutputTarget() == track) {
                existing->setOutputTarget(nullptr);
            }

            if (existing != track) {
                auto sends = existing->getSends();
                for (int i = (int)sends.size() - 1; i >= 0; --i) {
                    if (sends[(size_t)i].target == track) {
                        existing->removeSend(i);
                    }
                }
            }

            auto effects = existing->getEffects();
            if (!effects) continue;

            for (const auto& effect : *effects) {
                if (auto* comp = dynamic_cast<CompressorNode*>(effect.get())) {
                    if (comp->getSidechainSource() == removedNode) {
                        comp->setSidechainSource(nullptr);
                    }
                }
            }
        }

        auto it = std::remove(tracks.begin(), tracks.end(), track);
        if (it != tracks.end()) {
            tracks.erase(it, tracks.end());

            AudioGraphEvent event;
            event.type = AudioGraphEvent::TrackRemoved;
            event.track = track;
            sendEvent(event);

            rebuildGraph();
        }
    }

    bool AudioEngine::moveTrack(size_t fromIndex, size_t toIndex) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (fromIndex >= tracks.size() || toIndex >= tracks.size() || fromIndex == toIndex) {
            return false;
        }

        auto track = tracks[fromIndex];
        tracks.erase(tracks.begin() + static_cast<std::ptrdiff_t>(fromIndex));
        tracks.insert(tracks.begin() + static_cast<std::ptrdiff_t>(toIndex), track);
        rebuildGraph();
        return true;
    }

    bool AudioEngine::moveTrack(std::shared_ptr<AudioTrack> track, size_t toIndex) {
        if (!track) return false;

        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (toIndex >= tracks.size()) return false;

        auto it = std::find(tracks.begin(), tracks.end(), track);
        if (it == tracks.end()) return false;

        size_t fromIndex = static_cast<size_t>(std::distance(tracks.begin(), it));
        if (fromIndex == toIndex) return false;

        tracks.erase(it);
        tracks.insert(tracks.begin() + static_cast<std::ptrdiff_t>(toIndex), track);
        rebuildGraph();
        return true;
    }

    int AudioEngine::getTrackIndex(std::shared_ptr<AudioTrack> track) const {
        if (!track) return -1;

        std::shared_lock<std::shared_mutex> lock(graphMutex);
        auto it = std::find(tracks.begin(), tracks.end(), track);
        if (it == tracks.end()) return -1;
        return static_cast<int>(std::distance(tracks.begin(), it));
    }

    void AudioEngine::routeTrackTo(std::shared_ptr<AudioTrack> source, std::shared_ptr<AudioTrack> destination) {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        if (!source) return;

        if (destination && source->wouldCreateCycle(destination)) return;

        source->setOutputTarget(destination);


        AudioGraphEvent event;
        event.type = AudioGraphEvent::SetTrackOutput;
        event.track = source;
        event.targetTrack = destination;
        sendEvent(event);

        rebuildGraph();
    }

    void AudioEngine::clearTracks() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        tracks.clear();

        AudioGraphEvent event;
        event.type = AudioGraphEvent::ClearTracks;
        sendEvent(event);

        rebuildGraph();
    }

    void AudioEngine::updateSoloState() {
        std::unique_lock<std::shared_mutex> lock(graphMutex);
        updateSoloStateInternal();
    }

    void AudioEngine::updateSoloStateInternal() {
        bool anySolo = false;
        for (const auto& track : tracks) {
            if (track->isSolo()) {
                anySolo = true;
                break;
            }
        }

        if (!anySolo) {
            for (auto& track : tracks) {
                track->setSoloMuted(false);
            }
            return;
        }

        std::set<AudioTrack*> audible;
        std::set<AudioTrack*> contributing; // Tracks that are soloed or feed into soloed tracks

        for (const auto& track : tracks) {
            if (track->isSolo()) {
                audible.insert(track.get());
                contributing.insert(track.get());
            }
            if (track->isSoloSafe()) {
                audible.insert(track.get());
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;

            for (const auto& track : tracks) {
                AudioTrack* t = track.get();

                // Downstream: If a track is audible, its output target must be audible to reach Master
                if (audible.count(t)) {
                    auto target = t->getOutputTarget();
                    if (target && audible.find(target.get()) == audible.end()) {
                        audible.insert(target.get());
                        changed = true;
                    }
                }

                // Upstream: If a track is contributing (soloed or feeds a soloed track),
                // all its inputs are now also contributing and audible.
                if (contributing.count(t)) {
                    for (const auto& itTrack : tracks) {
                        AudioTrack* it = itTrack.get();
                        if (contributing.count(it)) continue;

                        bool feedsIn = (it->getOutputTarget() == track);
                        if (!feedsIn) {
                            for (const auto& send : it->getSends()) {
                                if (send.target == track) {
                                    feedsIn = true;
                                    break;
                                }
                            }
                        }

                        if (feedsIn) {
                            contributing.insert(it);
                            audible.insert(it);
                            changed = true;
                        }
                    }
                }
            }
        }

        for (auto& track : tracks) {
            track->setSoloMuted(audible.find(track.get()) == audible.end());
        }
    }

    void AudioEngine::sendEvent(const AudioGraphEvent& event) {
        while (!commandQueue.push(event)) {
            std::this_thread::yield();
        }
    }

    void AudioEngine::getScopeData(std::vector<float>& dest) {
        std::unique_lock<std::mutex> lock(scopeMutex);
        if (dest.size() != scopeBuffer.size()) {
            dest.resize(scopeBuffer.size());
        }


        size_t idx = scopeWriteIndex;
        for (size_t i = 0; i < scopeBuffer.size(); ++i) {
            dest[i] = scopeBuffer[idx];
            idx = (idx + 1) % scopeBuffer.size();
        }
    }

    void AudioEngine::flushCommands() {
        AudioGraphEvent event;
        while (commandQueue.pop(event)) {
            if (event.type == AudioGraphEvent::TrackAdded) {
                audioThreadTracks.push_back(event.track);
                if (masterNode) masterNode->connectInput(event.track.get());
            } else if (event.type == AudioGraphEvent::TrackRemoved) {
                auto it = std::remove(audioThreadTracks.begin(), audioThreadTracks.end(), event.track);
                if (it != audioThreadTracks.end()) {
                    audioThreadTracks.erase(it, audioThreadTracks.end());
                }
                if (masterNode) masterNode->disconnectInput(event.track.get());
            } else if (event.type == AudioGraphEvent::ClearTracks) {
                if (masterNode) {
                    for (auto& track : audioThreadTracks) {
                        masterNode->disconnectInput(track.get());
                    }
                }
                audioThreadTracks.clear();
            } else if (event.type == AudioGraphEvent::SetSampleRate) {
                if (masterNode) {
                    std::vector<AudioNode*> visited;
                    masterNode->propagateSampleRate(event.sampleRate, visited);
                }
                if (monitoringNode) {
                    std::vector<AudioNode*> visited;
                    monitoringNode->propagateSampleRate(event.sampleRate, visited);
                }
            } else if (event.type == AudioGraphEvent::SetBlockSize) {
                if (masterNode) {
                    std::vector<AudioNode*> visited;
                    masterNode->propagateBlockSize(event.blockSize, visited);
                }
                if (monitoringNode) {
                    monitoringNode->setFormat(2, event.blockSize);
                }
            } else if (event.type == AudioGraphEvent::AllNotesOff) {
                for (auto& track : audioThreadTracks) {
                    if (auto* inst = dynamic_cast<InstrumentTrack*>(track.get())) {
                        inst->allNotesOff();
                    }
                }
            } else if (event.type == AudioGraphEvent::SetTrackOutput) {
                if (event.track) {
                    if (masterNode) masterNode->disconnectInput(event.track.get());
                    for (auto& t : audioThreadTracks) {
                         if (t != event.track) t->disconnectInput(event.track.get());
                    }
                    if (event.targetTrack) {
                        event.targetTrack->connectInput(event.track.get());
                    } else {
                        if (masterNode) masterNode->connectInput(event.track.get());
                    }
                }
            } else if (event.type == AudioGraphEvent::ClearTempoMarkers) {
                audioThreadTempoMarkers.clear();
            } else if (event.type == AudioGraphEvent::AddTempoMarker) {
                audioThreadTempoMarkers.push_back({event.position, event.bpm});
                std::sort(audioThreadTempoMarkers.begin(), audioThreadTempoMarkers.end(),
                          [](const TempoMarker& a, const TempoMarker& b){ return a.position < b.position; });
            } else if (event.type == AudioGraphEvent::UpdateGraph) {
                if (currentProcessingGraph) {
                    releaseQueue.push(currentProcessingGraph);
                }
                currentProcessingGraph = event.newGraph;
                if (currentProcessingGraph) {
                    for (const auto& pair : currentProcessingGraph->latencies) {
                         if (pair.first) pair.first->setOutputLatency(pair.second);
                    }
                }
            }
        }
    }

    void AudioEngine::process(float* outputBuffer, const float* inputBuffer, size_t numFrames, size_t numChannels) {

        auto startTime = std::chrono::steady_clock::now();

        flushCommands();

        if (isExporting.load()) {
            std::memset(outputBuffer, 0, numFrames * numChannels * sizeof(float));
            return;
        }


        // Solo state is updated via updateSoloState() from message thread or rebuildGraph

        TransportState currentState = state.load();

        bool isPlaying = (currentState == TransportState::Playing || currentState == TransportState::Recording);
        bool isRecording = (currentState == TransportState::Recording);

        if (!masterNode) {
            std::memset(outputBuffer, 0, numFrames * numChannels * sizeof(float));
            return;
        }


        if (isRecording && inputBuffer) {
             for (auto& track : audioThreadTracks) {
                 if (track->isArmed()) {
                     track->appendRecordingData(inputBuffer, numFrames, numChannels);
                 }
             }
        }


        uint64_t startFrame = currentFrameIndex.load();

        float blockBpm = projectBpm.load();
        if (!audioThreadTempoMarkers.empty()) {
            for (auto it = audioThreadTempoMarkers.rbegin(); it != audioThreadTempoMarkers.rend(); ++it) {
                if (it->position <= startFrame) {
                    blockBpm = it->bpm;
                    break;
                }
            }
        }
        playbackBpm.store(blockBpm);


        gHostContext.bpm = blockBpm;
        gHostContext.sampleRate = sampleRate;
        gHostContext.projectTimeSamples = startFrame;
        gHostContext.playing = isPlaying;
        gHostContext.timeSigNumerator = 4.0;
        gHostContext.timeSigDenominator = 4.0;
        gHostContext.loopActive = looping;
        gHostContext.loopStart = loopStart;
        gHostContext.loopEnd = loopEnd;


        engineFrameCount += numFrames;


        HostContext currentContext = gHostContext;

        if (currentProcessingGraph) {
            for (const auto& layer : currentProcessingGraph->layers) {
                if (layer.empty()) continue;
                if (layer.size() == 1 || numFrames < 128) {
                    for (auto& track : layer) {
                        track->getOutput(engineFrameCount, startFrame);
                    }
                } else {
                    gHostContext = currentContext;

                    std::atomic<size_t> remaining(layer.size());
                    for (const auto& track : layer) {
                        AudioTrack* t = track.get();
                        ThreadPool::global().enqueue_void([t, this, startFrame, currentContext, &remaining] {
                            gHostContext = currentContext;
                            t->getOutput(engineFrameCount, startFrame);
                            remaining.fetch_sub(1, std::memory_order_release);
                        });
                    }
                    while (remaining.load(std::memory_order_acquire) > 0) {
                        std::this_thread::yield();
                    }
                }
            }
        }

        const AudioBlock& block = masterNode->getOutput(engineFrameCount, startFrame);


        bool monitoring = SettingsManager::get().presetMonitoringEnabled;
        if (monitoring && monitoringNode && monitoringNode->isActive()) {

            AudioBlock& mutableBlock = const_cast<AudioBlock&>(block);
            monitoringNode->process(mutableBlock, engineFrameCount, startFrame);
        }


        if (isPlaying) {
            currentFrameIndex += numFrames;


            if (looping && loopEnd > loopStart) {
                if (currentFrameIndex >= loopEnd) {
                    currentFrameIndex = loopStart;

                    AudioGraphEvent loopEvent;
                    loopEvent.type = AudioGraphEvent::AllNotesOff;
                    sendEvent(loopEvent);
                }
            }
        }


        size_t samplesToCopy = std::min(numFrames, block.getSampleCount());
        size_t srcChannels = block.getChannelCount();

        float* outPtr = outputBuffer;

        for (size_t i = 0; i < samplesToCopy; ++i) {
            for (size_t ch = 0; ch < numChannels; ++ch) {
                float sample = 0.0f;
                if (ch < srcChannels) {
                    sample = block.getChannelPointer(ch)[i];
                }
                *outPtr++ = sample;
            }
        }


        if (samplesToCopy < numFrames) {
            std::memset(outputBuffer + samplesToCopy * numChannels, 0, (numFrames - samplesToCopy) * numChannels * sizeof(float));
        }


        if (metronomeEnabled && isPlaying && samplesToCopy > 0) {
             double bpmVal = blockBpm;
             if (bpmVal > 0.0) {
                 double framesPerBeat = (60.0 / bpmVal) * sampleRate;
                 int durFrames = (int)(0.1 * sampleRate);


                 float inc1200 = 1200.0f * (float)METRONOME_LUT_SIZE / (float)sampleRate;
                 float inc800 = 800.0f * (float)METRONOME_LUT_SIZE / (float)sampleRate;


                 double startBeatPos = (double)startFrame / framesPerBeat;
                 double currentBeatIndexD;
                 double currentPhase = std::modf(startBeatPos, &currentBeatIndexD);
                 double phaseInc = 1.0 / framesPerBeat;
                 double phaseThreshold = (double)durFrames / framesPerBeat;
                 float currentMetronomeVol = metronomeVolume.load();
                 float invDurFrames = 1.0f / (float)durFrames;

                 for (size_t i = 0; i < samplesToCopy; ++i) {
                     if (currentPhase < phaseThreshold) {
                         uint64_t beatIndex = (uint64_t)currentBeatIndexD;
                         bool isBar = (beatIndex % 4 == 0);

                         uint64_t framesSinceBeat = (uint64_t)(currentPhase * framesPerBeat);

                         float inc = isBar ? inc1200 : inc800;
                         size_t idx = (size_t)(framesSinceBeat * inc) & METRONOME_LUT_MASK;

                         float env = 1.0f - ((float)framesSinceBeat * invDurFrames);
                         if (env < 0.0f) env = 0.0f;

                         float sample = metronomeLUT[idx] * env * currentMetronomeVol;

                         for(size_t ch=0; ch<numChannels; ++ch) {
                             outputBuffer[i*numChannels + ch] += sample;
                         }
                     }

                     currentPhase += phaseInc;
                     if (currentPhase >= 1.0) {
                         currentPhase -= 1.0;
                         currentBeatIndexD += 1.0;
                     }
                 }
             }
        }


        const size_t totalSamples = numFrames * numChannels;
        for (size_t i = 0; i < totalSamples; ++i) {
             float s = outputBuffer[i];
             if (s < -1.0f) s = -1.0f;
             else if (s > 1.0f) s = 1.0f;
             outputBuffer[i] = s;
        }


        if (numFrames > 0) {
            std::unique_lock<std::mutex> lock(scopeMutex, std::try_to_lock);
            if (lock.owns_lock()) {
                for (size_t i = 0; i < numFrames; ++i) {
                    float sample = 0.0f;

                    for (size_t ch = 0; ch < numChannels; ++ch) {
                        sample += outputBuffer[i * numChannels + ch];
                    }
                    if (numChannels > 0) sample /= (float)numChannels;

                    scopeBuffer[scopeWriteIndex] = sample;
                    scopeWriteIndex = (scopeWriteIndex + 1) % scopeBuffer.size();
                }
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = endTime - startTime;

        if (sampleRate > 0.0 && numFrames > 0) {
            double availableTime = (double)numFrames / sampleRate;
            double currentLoad = diff.count() / availableTime;

            rtLoad.store(currentLoad);

            double oldLoad = cpuLoad.load();
            cpuLoad.store(oldLoad * 0.95 + currentLoad * 0.05);
        }
    }

    bool AudioEngine::renderProject(const ExportConfig& config)
    {
        lastExportError.clear();

        if (config.outputFile.empty()) {
            lastExportError = "Output path is empty.";
            return false;
        }
        if (!masterNode) {
            lastExportError = "Master node is not available.";
            return false;
        }

        const double sourceSampleRate = sampleRate;
        const double outputSampleRate = (config.targetSampleRate > 0.0) ? config.targetSampleRate : sourceSampleRate;
        if (outputSampleRate <= 0.0 || sourceSampleRate <= 0.0) {
            lastExportError = "Invalid sample rate.";
            return false;
        }

        uint64_t projectEndFrame = 0;
        uint64_t localLoopStart = 0;
        uint64_t localLoopEnd = 0;
        uint64_t localCursor = 0;
        float localBpm = projectBpm.load();

        {
            std::shared_lock<std::shared_mutex> lock(graphMutex);
            localLoopStart = loopStart;
            localLoopEnd = loopEnd;
            localCursor = currentFrameIndex.load();
            for (const auto& track : tracks) {
                if (auto clips = track->getClips()) {
                    for (const auto& clip : *clips) {
                        uint64_t end = clip->getStartFrame() + clip->getLengthFrames();
                        if (end > projectEndFrame) projectEndFrame = end;
                    }
                }
                if (auto inst = std::dynamic_pointer_cast<InstrumentTrack>(track)) {
                    if (auto clips = inst->getMidiClips()) {
                        for (const auto& clip : *clips) {
                            uint64_t end = clip->getStartFrame() + clip->getLengthFrames();
                            if (end > projectEndFrame) projectEndFrame = end;
                        }
                    }
                }
            }
        }

        if (projectEndFrame == 0) {
            lastExportError = "Project has no clips to render.";
            return false;
        }

        uint64_t startFrame = 0;
        uint64_t endFrame = projectEndFrame;
        if (config.range == ExportRange::LoopRegion) {
            if (localLoopEnd <= localLoopStart) {
                lastExportError = "Loop range is empty.";
                return false;
            }
            startFrame = localLoopStart;
            endFrame = localLoopEnd;
        } else if (config.range == ExportRange::FromCursor) {
            startFrame = std::min(localCursor, projectEndFrame);
            endFrame = projectEndFrame;
        }

        if (endFrame <= startFrame) {
            lastExportError = "Export range is empty.";
            return false;
        }

        if (config.includeTail) {
            endFrame += static_cast<uint64_t>(2.0 * sourceSampleRate);
        }

        juce::File destination(config.outputFile);
        destination.getParentDirectory().createDirectory();

        juce::File wavTarget = destination;
        juce::File tempWav;
        const bool wantsMp3 = (config.format == ExportFormat::Mp3);
        if (wantsMp3) {
            tempWav = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("orion_export_" + juce::String::toHexString((int64_t)juce::Time::getMillisecondCounterHiRes()) + ".wav");
            wavTarget = tempWav;
        }

        destination.deleteFile();
        wavTarget.deleteFile();

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        auto* wavFormat = formatManager.findFormatForFileExtension("wav");
        if (!wavFormat) {
            lastExportError = "WAV writer is not available.";
            return false;
        }

        auto* stream = new juce::FileOutputStream(wavTarget);
        if (!stream->openedOk()) {
            delete stream;
            lastExportError = "Unable to open destination file for writing.";
            return false;
        }

        const int wavBitDepth = wantsMp3 ? 16 : config.bitDepth;
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat->createWriterFor(
            stream,
            outputSampleRate,
            2,
            wavBitDepth,
            {},
            0
        ));
        if (!writer) {
            delete stream;
            lastExportError = "Unable to create audio writer.";
            return false;
        }

        struct ExportFlagScope {
            std::atomic<bool>& flag;
            explicit ExportFlagScope(std::atomic<bool>& f) : flag(f) { flag.store(true); }
            ~ExportFlagScope() { flag.store(false); }
        } exportScope(isExporting);

        shouldAbortOfflineRender = false;

        const size_t renderBlockSize = std::max<size_t>(64, blockSize);
        juce::AudioBuffer<float> sourceChunk(2, (int)renderBlockSize);
        juce::AudioBuffer<float> outputChunk(2, 8192);

        std::array<juce::LagrangeInterpolator, 2> resamplers;
        for (auto& rs : resamplers) rs.reset();
        std::array<std::vector<float>, 2> pending;

        double renderCpu = 0.0;
        double writeCpu = 0.0;
        uint64_t exportCacheKey = (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();
        const bool needsResample = std::abs(outputSampleRate - sourceSampleRate) > 1.0e-9;
        const double ratio = sourceSampleRate / outputSampleRate;
        uint64_t frame = startFrame;

        while (frame < endFrame || (!pending[0].empty() && needsResample)) {
            if (shouldAbortOfflineRender) {
                lastExportError = "Export canceled.";
                return false;
            }

            if (frame < endFrame) {
                size_t numFrames = std::min<uint64_t>(renderBlockSize, endFrame - frame);
                auto renderStart = std::chrono::steady_clock::now();
                {
                    std::shared_lock<std::shared_mutex> lock(graphMutex);
                    gHostContext.bpm = localBpm;
                    gHostContext.sampleRate = sourceSampleRate;
                    gHostContext.projectTimeSamples = frame;
                    gHostContext.playing = true;
                    const AudioBlock& block = masterNode->getOutput(exportCacheKey++, frame);
                    sourceChunk.clear();
                    for (int ch = 0; ch < 2; ++ch) {
                        if (ch < (int)block.getChannelCount()) {
                            sourceChunk.copyFrom(ch, 0, block.getChannelPointer(ch), (int)numFrames);
                        } else if (block.getChannelCount() > 0) {
                            sourceChunk.copyFrom(ch, 0, block.getChannelPointer(0), (int)numFrames);
                        }
                    }
                    if (!needsResample) {
                        auto writeStart = std::chrono::steady_clock::now();
                        writer->writeFromAudioSampleBuffer(sourceChunk, 0, (int)numFrames);
                        auto writeEnd = std::chrono::steady_clock::now();
                        writeCpu += std::chrono::duration<double>(writeEnd - writeStart).count();
                    }
                }
                auto renderEnd = std::chrono::steady_clock::now();
                renderCpu += std::chrono::duration<double>(renderEnd - renderStart).count();

                if (needsResample) {
                    pending[0].insert(pending[0].end(), sourceChunk.getReadPointer(0), sourceChunk.getReadPointer(0) + numFrames);
                    pending[1].insert(pending[1].end(), sourceChunk.getReadPointer(1), sourceChunk.getReadPointer(1) + numFrames);
                }

                frame += numFrames;
            }

            if (!needsResample) {
                continue;
            }

            int available = (int)std::min(pending[0].size(), pending[1].size());
            int maxOut = (int)outputChunk.getNumSamples();
            int outSamples = (available > 8)
                ? std::min(maxOut, (int)std::floor((available - 4) / ratio))
                : 0;

            if (frame >= endFrame && available > 4) {
                outSamples = std::max(outSamples, std::min(maxOut, (int)std::ceil((available - 4) / ratio)));
            }

            if (outSamples <= 0) {
                if (frame >= endFrame) break;
                continue;
            }

            int usedLeft = 0;
            int usedRight = 0;
            float* outL = outputChunk.getWritePointer(0);
            float* outR = outputChunk.getWritePointer(1);
            usedLeft = resamplers[0].process(ratio, pending[0].data(), outL, outSamples);
            usedRight = resamplers[1].process(ratio, pending[1].data(), outR, outSamples);
            const int used = std::min(usedLeft, usedRight);

            if (used > 0) {
                pending[0].erase(pending[0].begin(), pending[0].begin() + used);
                pending[1].erase(pending[1].begin(), pending[1].begin() + used);
            }

            auto writeStart = std::chrono::steady_clock::now();
            writer->writeFromAudioSampleBuffer(outputChunk, 0, outSamples);
            auto writeEnd = std::chrono::steady_clock::now();
            writeCpu += std::chrono::duration<double>(writeEnd - writeStart).count();
        }

        writer.reset();
        exportRenderCpuSeconds.store(renderCpu);
        exportWriteCpuSeconds.store(writeCpu);

        if (wantsMp3) {
            std::string err;
            const int bitrate = static_cast<int>(config.mp3Bitrate);
            if (!transcodeWithLame(wavTarget, destination, bitrate, err)) {
                lastExportError = err.empty() ? "MP3 encoding failed." : err;
                wavTarget.deleteFile();
                return false;
            }
            wavTarget.deleteFile();
        } else if (!wavTarget.existsAsFile() || wavTarget.getSize() <= 0) {
            lastExportError = "Rendered WAV file is empty.";
            return false;
        }

        return true;
    }

    bool AudioEngine::renderProject(const std::string& outputFile,
                                    const std::string& format,
                                    ExportRange range,
                                    bool includeTail,
                                    double targetSampleRate,
                                    int bitDepth)
    {
        ExportConfig config;
        config.outputFile = outputFile;
        std::string lowered = format;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        config.format = (lowered == "mp3") ? ExportFormat::Mp3 : ExportFormat::Wav;
        config.range = range;
        config.includeTail = includeTail;
        config.targetSampleRate = targetSampleRate;
        config.bitDepth = bitDepth;
        return renderProject(config);
    }

    void AudioEngine::rebuildGraph() {
        if (rebuildSuspended.load()) return;

        std::shared_ptr<ProcessingGraph> oldGraph;
        while (releaseQueue.pop(oldGraph)) {}

        auto newGraph = std::make_shared<ProcessingGraph>();

        if (!masterNode) return;

        std::map<AudioTrack*, std::vector<AudioTrack*>> inputsMap;
        std::map<AudioTrack*, int> inDegree;
        std::map<AudioTrack*, std::vector<AudioTrack*>> dependents;
        std::vector<AudioTrack*> roots;
        std::map<AudioTrack*, std::shared_ptr<AudioTrack>> trackPtrMap;

        for (const auto& track : tracks) {
            trackPtrMap[track.get()] = track;
            inDegree[track.get()] = 0;
        }

        for (const auto& track : tracks) {
            AudioTrack* t = track.get();

            auto target = t->getOutputTarget();
            if (target) {
                inputsMap[target.get()].push_back(t);
                dependents[t].push_back(target.get());
                inDegree[target.get()]++;
            } else {
                roots.push_back(t);
            }

            auto effects = t->getEffects();
            if (effects) {
                for (const auto& effect : *effects) {
                    if (auto* comp = dynamic_cast<CompressorNode*>(effect.get())) {
                        AudioNode* source = comp->getSidechainSource();
                        if (!source) continue;

                        AudioTrack* sourceTrack = nullptr;
                        for (const auto& candidate : tracks) {
                            if (candidate.get() == source) {
                                sourceTrack = candidate.get();
                                break;
                            }
                        }

                        if (!sourceTrack || sourceTrack == t) {
                            comp->setSidechainSource(nullptr);
                            continue;
                        }

                        if (inDegree.count(sourceTrack)) {
                            dependents[sourceTrack].push_back(t);
                            inDegree[t]++;
                        }
                    }
                }
            }

            auto sends = t->getSends();
            for (const auto& send : sends) {
                if (send.target) {
                    AudioTrack* targetTrack = send.target.get();
                    inputsMap[targetTrack].push_back(t);

                    if (inDegree.count(targetTrack)) {
                        dependents[t].push_back(targetTrack);
                        inDegree[targetTrack]++;
                    }
                }
            }
        }

        std::queue<AudioTrack*> q;
        for (const auto& pair : inDegree) {
            if (pair.second == 0) q.push(pair.first);
        }

        size_t processedNodes = 0;
        while (!q.empty()) {
            std::vector<std::shared_ptr<AudioTrack>> layer;
            size_t layerSize = q.size();
            for (size_t i = 0; i < layerSize; ++i) {
                AudioTrack* t = q.front();
                q.pop();

                if (trackPtrMap.count(t)) {
                    layer.push_back(trackPtrMap[t]);
                    processedNodes++;
                }

                if (dependents.count(t)) {
                    for (AudioTrack* dep : dependents[t]) {
                        inDegree[dep]--;
                        if (inDegree[dep] == 0) q.push(dep);
                    }
                }
            }
            newGraph->layers.push_back(layer);
        }

        if (processedNodes < tracks.size()) {
            std::vector<std::shared_ptr<AudioTrack>> cycleLayer;
            cycleLayer.reserve(tracks.size() - processedNodes);
            for (const auto& track : tracks) {
                if (inDegree[track.get()] > 0) {
                    cycleLayer.push_back(track);
                }
            }
            if (!cycleLayer.empty()) {
                newGraph->layers.push_back(cycleLayer);
            }
        }

        std::map<AudioTrack*, int> naturalLatencyCache;

        std::function<int(AudioTrack*)> computeNaturalLatency;
        computeNaturalLatency = [&](AudioTrack* t) -> int {
            if (naturalLatencyCache.count(t)) return naturalLatencyCache[t];

            int maxInputLatency = 0;
            if (inputsMap.count(t)) {
                for (auto* input : inputsMap[t]) {
                    int l = computeNaturalLatency(input);
                    if (l > maxInputLatency) maxInputLatency = l;
                }
            }

            int nl = t->getLatency() + maxInputLatency;
            naturalLatencyCache[t] = nl;
            return nl;
        };

        int globalMaxLatency = 0;
        for (auto* root : roots) {
            int l = computeNaturalLatency(root);
            if (l > globalMaxLatency) globalMaxLatency = l;
        }

        for (const auto& track : tracks) {
            int requiredTime = globalMaxLatency;

            std::vector<AudioTrack*> consumers;
            auto target = track->getOutputTarget();
            if (target) consumers.push_back(target.get());

            auto sends = track->getSends();
            for (const auto& send : sends) {
                if (send.target) consumers.push_back(send.target.get());
            }

            if (!consumers.empty()) {
                requiredTime = 0;
                for (auto* consumer : consumers) {
                    int maxSiblingLatency = 0;
                    if (inputsMap.count(consumer)) {
                        for (auto* sibling : inputsMap[consumer]) {
                            int l = 0;
                            if (naturalLatencyCache.count(sibling)) {
                                l = naturalLatencyCache[sibling];
                            } else {
                                l = computeNaturalLatency(sibling);
                            }
                            if (l > maxSiblingLatency) maxSiblingLatency = l;
                        }
                    }
                    if (maxSiblingLatency > requiredTime) requiredTime = maxSiblingLatency;
                }
            }

            int nl = 0;
            if (naturalLatencyCache.count(track.get())) {
                nl = naturalLatencyCache[track.get()];
            } else {
                nl = computeNaturalLatency(track.get());
            }

            int comp = requiredTime - nl;
            if (comp < 0) comp = 0;

            newGraph->latencies.push_back({track, comp});
        }

        AudioGraphEvent event;
        event.type = AudioGraphEvent::UpdateGraph;
        event.newGraph = newGraph;
        sendEvent(event);

        updateSoloStateInternal();
    }

    bool AudioEngine::renderPreview(const std::string& outputFile)
    {
        flushCommands();
        float len = SettingsManager::get().previewLength;
        uint64_t maxFrame = (uint64_t)(len * sampleRate);


        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        juce::AudioFormat* fmt = formatManager.findFormatForFileExtension("wav");

        if (!fmt) return false;

        juce::File file(outputFile);
        file.deleteFile();

        std::unique_ptr<juce::AudioFormatWriter> writer(fmt->createWriterFor(
            new juce::FileOutputStream(file),
            sampleRate,
            2,
            16,
            {},
            0
        ));

        if (!writer) return false;

        struct ExportScope {
            std::atomic<bool>& flag;
            std::atomic<bool>& abort;
            explicit ExportScope(std::atomic<bool>& f, std::atomic<bool>& a) : flag(f), abort(a) {
                flag.store(true);
                abort.store(false);
            }
            ~ExportScope() { flag.store(false); }
        } scope(isExporting, shouldAbortOfflineRender);

        bool oldLooping = looping;
        looping = false;


        size_t renderBlockSize = blockSize;
        juce::AudioBuffer<float> buffer(2, (int)renderBlockSize);
        AudioBlock blockCopy;


        uint64_t exportCacheKey = (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();

        for (uint64_t frame = 0; frame < maxFrame; frame += renderBlockSize) {
            if (shouldAbortOfflineRender.load()) break;

            size_t numFrames = std::min((size_t)(maxFrame - frame), renderBlockSize);


            gHostContext.bpm = projectBpm.load();
            gHostContext.sampleRate = sampleRate;
            gHostContext.projectTimeSamples = frame;
            gHostContext.playing = true;

            {
                std::shared_lock<std::shared_mutex> lock(graphMutex);
                const AudioBlock& block = masterNode->getOutput(exportCacheKey++, frame);
                blockCopy.resize(block.getChannelCount(), block.getSampleCount());
                blockCopy.copyFrom(block);
            }


            buffer.clear();
            for (int ch=0; ch<2; ++ch) {
                if (ch < (int)blockCopy.getChannelCount()) {
                    buffer.copyFrom(ch, 0, blockCopy.getChannelPointer(ch), (int)numFrames);
                }
            }

            writer->writeFromAudioSampleBuffer(buffer, 0, (int)numFrames);
        }

        writer.reset();
        if (shouldAbortOfflineRender.load()) {
            file.deleteFile();
        }

        looping = oldLooping;

        return true;
    }

    void AudioEngine::processPhysicalMidi(const juce::MidiMessage& message) {
        std::shared_lock<std::shared_mutex> lock(graphMutex);
        for (const auto& track : tracks) {
            if (track->isArmed()) {
                if (auto* instTrack = dynamic_cast<InstrumentTrack*>(track.get())) {
                    instTrack->injectLiveMidiMessage(message);
                }
            }
        }
    }
}
