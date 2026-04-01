#pragma once

#include "AudioNode.h"
#include "AudioTrack.h"
#include "MasterNode.h"
#include "EnvironmentNode.h"
#include "RingBuffer.h"
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <future>

namespace Orion {

    extern std::atomic<int> gDiskActivityCounter;

    struct ProcessingGraph {
        std::vector<std::vector<std::shared_ptr<AudioTrack>>> layers;
        std::vector<std::pair<std::shared_ptr<AudioTrack>, int>> latencies;
    };

    struct AudioGraphEvent {
        enum Type {
            TrackAdded,
            TrackRemoved,
            ClearTracks,
            SetSampleRate,
            SetBlockSize,
            AllNotesOff,
            SetTrackOutput,
            AddTempoMarker,
            ClearTempoMarkers,
            UpdateGraph
        } type;
        std::shared_ptr<AudioTrack> track;
        std::shared_ptr<AudioTrack> targetTrack;
        double sampleRate = 0.0;
        size_t blockSize = 0;


        uint64_t position = 0;
        float bpm = 0.0f;

        std::shared_ptr<ProcessingGraph> newGraph;
    };

    enum class TransportState {
        Stopped,
        Playing,
        Paused,
        Recording
    };

    enum class ExportRange {
        FullProject,
        LoopRegion,
        FromCursor
    };

    enum class ExportFormat {
        Wav,
        Mp3
    };

    enum class Mp3BitrateKbps {
        Kbps128 = 128,
        Kbps192 = 192,
        Kbps256 = 256,
        Kbps320 = 320
    };

    struct ExportConfig {
        std::string outputFile;
        ExportFormat format = ExportFormat::Wav;
        ExportRange range = ExportRange::FullProject;
        bool includeTail = true;
        double targetSampleRate = 0.0;
        int bitDepth = 24;
        Mp3BitrateKbps mp3Bitrate = Mp3BitrateKbps::Kbps320;
    };

    class InstrumentTrack;

    class AudioEngine {
    public:
        struct TempoMarker {
            uint64_t position;
            float bpm;
        };

        struct TimeSigMarker {
            uint64_t position;
            int numerator;
            int denominator;
        };

        struct RegionMarker {
            uint64_t position;
            std::string name;

        };

        AudioEngine();
        ~AudioEngine() = default;

        void setSampleRate(double rate);
        void setBlockSize(size_t size);

        double getSampleRate() const { return sampleRate; }
        size_t getBlockSize() const { return blockSize; }


        void play();
        void pause();
        void stop();
        void panic();
        void record();

        void setCursor(uint64_t frame);
        uint64_t getCursor() const { return currentFrameIndex.load(); }
        TransportState getTransportState() const { return state.load(); }

        bool isLooping() const { return looping; }
        void setLooping(bool loop) { looping = loop; }
        void setLoopPoints(uint64_t start, uint64_t end) { loopStart = start; loopEnd = end; }
        uint64_t getLoopStart() const { return loopStart; }
        uint64_t getLoopEnd() const { return loopEnd; }

        void setBpm(float newBpm) { projectBpm = newBpm; playbackBpm = newBpm; }
        float getBpm() const { return projectBpm.load(); }
        float getPlaybackBpm() const { return playbackBpm.load(); }

        void setKeySignature(const std::string& key) { std::unique_lock<std::shared_mutex> lock(graphMutex); keySignature = key; }
        std::string getKeySignature() const { std::shared_lock<std::shared_mutex> lock(graphMutex); return keySignature; }

        void setAuthor(const std::string& name) { std::unique_lock<std::shared_mutex> lock(graphMutex); author = name; }
        std::string getAuthor() const { std::shared_lock<std::shared_mutex> lock(graphMutex); return author; }

        void setGenre(const std::string& name) { std::unique_lock<std::shared_mutex> lock(graphMutex); genre = name; }
        std::string getGenre() const { std::shared_lock<std::shared_mutex> lock(graphMutex); return genre; }

        void setTimeSignature(int num, int den) { std::unique_lock<std::shared_mutex> lock(graphMutex); timeSigNumerator = num; timeSigDenominator = den; }
        void getTimeSignature(int& num, int& den) const { std::shared_lock<std::shared_mutex> lock(graphMutex); num = timeSigNumerator; den = timeSigDenominator; }

        bool isMetronomeEnabled() const { return metronomeEnabled; }
        void setMetronomeEnabled(bool enabled) { metronomeEnabled = enabled; }

        float getMetronomeVolume() const { return metronomeVolume.load(); }
        void setMetronomeVolume(float volume) { metronomeVolume = volume; }


        void addTempoMarker(uint64_t position, float newBpm);
        void removeTempoMarker(int index);
        void clearMarkers();
        const std::vector<TempoMarker>& getTempoMarkers() const;

        void addTimeSigMarker(uint64_t position, int num, int den);
        void removeTimeSigMarker(int index);
        const std::vector<TimeSigMarker>& getTimeSigMarkers() const;

        void addRegionMarker(uint64_t position, const std::string& name);
        void removeRegionMarker(int index);
    void updateRegionMarker(int index, uint64_t position, const std::string& name);
        const std::vector<RegionMarker>& getRegionMarkers() const;


        std::shared_ptr<AudioTrack> createTrack();
        std::shared_ptr<InstrumentTrack> createInstrumentTrack();
        std::shared_ptr<AudioTrack> createAuxTrack();
        std::shared_ptr<AudioTrack> createBusTrack();

        void addTrack(std::shared_ptr<AudioTrack> track);
        void removeTrack(std::shared_ptr<AudioTrack> track);
        bool moveTrack(size_t fromIndex, size_t toIndex);
        bool moveTrack(std::shared_ptr<AudioTrack> track, size_t toIndex);
        int getTrackIndex(std::shared_ptr<AudioTrack> track) const;
        const std::vector<std::shared_ptr<AudioTrack>>& getTracks() const { return tracks; }
        void clearTracks();


        void routeTrackTo(std::shared_ptr<AudioTrack> source, std::shared_ptr<AudioTrack> destination);


        MasterNode* getMasterNode() const { return masterNode.get(); }

        EnvironmentNode* getMonitoringNode() const { return monitoringNode.get(); }


        double getCpuLoad() const { return cpuLoad.load(); }
        double getRtLoad() const { return rtLoad.load(); }


        void lockGraph() { graphMutex.lock(); }
        void unlockGraph() { graphMutex.unlock(); }


        void updateSoloState();

        void suspendGraphRebuild() { rebuildSuspended = true; }
        void resumeGraphRebuild() { rebuildSuspended = false; rebuildGraph(); }

        void flushCommands();

        void process(float* outputBuffer, const float* inputBuffer, size_t numFrames, size_t numChannels);


        void getScopeData(std::vector<float>& dest);


        bool renderProject(const ExportConfig& config);
        const std::string& getLastExportError() const { return lastExportError; }

        bool renderProject(const std::string& outputFile,
                           const std::string& format,
                           ExportRange range = ExportRange::FullProject,
                           bool includeTail = true,
                           double targetSampleRate = 0.0,
                           int bitDepth = 24);


        bool renderPreview(const std::string& outputFile);
        void cancelPreviewRender() { shouldAbortOfflineRender = true; }
        bool isPreviewRenderCancelled() const { return shouldAbortOfflineRender.load(); }


        void processPhysicalMidi(const juce::MidiMessage& message);

    private:
        std::atomic<bool> shouldAbortOfflineRender { false };
        double sampleRate;
        size_t blockSize;

        std::atomic<uint64_t> currentFrameIndex;
        std::atomic<uint64_t> playStartPosition { 0 };
        std::atomic<TransportState> state;
        std::atomic<float> projectBpm { 120.0f };
        std::atomic<float> playbackBpm { 120.0f };

        std::string keySignature = "C Maj";
        std::string author = "";
        std::string genre = "";
        int timeSigNumerator = 4;
        int timeSigDenominator = 4;

        bool metronomeEnabled = false;
        std::atomic<float> metronomeVolume { 0.5f };

        bool looping;
        uint64_t loopStart, loopEnd;

        std::shared_ptr<MasterNode> masterNode;
        std::shared_ptr<EnvironmentNode> monitoringNode;

        std::vector<std::shared_ptr<AudioTrack>> tracks;
        mutable std::shared_mutex graphMutex;


        RingBuffer<AudioGraphEvent> commandQueue;
        std::vector<std::shared_ptr<AudioTrack>> audioThreadTracks;
        std::vector<TempoMarker> audioThreadTempoMarkers;

        std::atomic<double> cpuLoad { 0.0 };
        std::atomic<double> rtLoad { 0.0 };
        std::atomic<double> exportRenderCpuSeconds { 0.0 };
        std::atomic<double> exportWriteCpuSeconds { 0.0 };

        uint64_t engineFrameCount = 0;


        std::mutex scopeMutex;
        std::vector<float> scopeBuffer;
        size_t scopeWriteIndex = 0;

        std::atomic<bool> isExporting { false };
        std::string lastExportError;

        void sendEvent(const AudioGraphEvent& event);

        std::vector<TempoMarker> tempoMarkers;
        std::vector<TimeSigMarker> timeSigMarkers;
        std::vector<RegionMarker> regionMarkers;


        void rebuildGraph();
        void updateSoloStateInternal();

        std::atomic<bool> rebuildSuspended { false };

        std::shared_ptr<ProcessingGraph> currentProcessingGraph;
        RingBuffer<std::shared_ptr<ProcessingGraph>> releaseQueue;
        std::vector<std::future<void>> workerFutures;
    };

}
