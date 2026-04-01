#include "orion/ProjectSerializer.h"
#include "orion/AudioTrack.h"
#include "orion/InstrumentTrack.h"
#include "orion/AudioClip.h"
#include "orion/MidiClip.h"
#include "orion/DelayNode.h"
#include "orion/EQ3Node.h"
#include "orion/ReverbNode.h"
#include "orion/CompressorNode.h"
#include "orion/LimiterNode.h"
#include "orion/SynthesizerNode.h"
#include "orion/SamplerNode.h"
#include "orion/FluxShaperNode.h"
#include "orion/PrismStackNode.h"
#include "orion/VST3Node.h"
#include "orion/VSTNode.h"
#include "orion/PluginManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace Orion {

namespace {
    static const std::string base64_chars =
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789+/";

    static const unsigned char base64_lookup[256] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    std::string base64_encode(const std::vector<char>& buf) {
        std::string ret;
        ret.reserve((buf.size() + 2) / 3 * 4);
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (char c : buf) {
            char_array_3[i++] = (unsigned char)c;
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for(j = i; j < 3; j++) char_array_3[j] = '\0';
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
            while((i++ < 3)) ret += '=';
        }
        return ret;
    }

    std::vector<char> base64_decode(const std::string& encoded_string) {
        int in_len = (int)encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<char> ret;
        ret.reserve(in_len * 3 / 4);

        while (in_len-- && (encoded_string[in_] != '=')) {
            unsigned char c = (unsigned char)encoded_string[in_++];
            unsigned char val = base64_lookup[c];
            if (val != 0xFF) {
                char_array_4[i++] = val;
                if (i == 4) {
                    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                    for (i = 0; (i < 3); i++) ret.push_back(char_array_3[i]);
                    i = 0;
                }
            }
        }

        if (i) {
            for (j = i; j < 4; j++) char_array_4[j] = 0;
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
        }
        return ret;
    }

    bool isVersionOlderThan(const std::string& ver, int major, int minor, int patch) {
        int v[3] = {0, 0, 0};
        char dot;
        std::stringstream ss(ver);

        if (ss >> v[0]) {
            if (ss >> dot >> v[1]) {
                ss >> dot >> v[2];
            }
        }

        if (v[0] < major) return true;
        if (v[0] > major) return false;
        if (v[1] < minor) return true;
        if (v[1] > minor) return false;
        if (v[2] < patch) return true;
        return false;
    }

    json serializeEffect(std::shared_ptr<EffectNode> eff) {
        json jEff;
        if (auto vst = std::dynamic_pointer_cast<VST3Node>(eff)) {
            jEff["type"] = "VST3";
            jEff["path"] = vst->getPath();
            jEff["cid"] = vst->getCID();
        } else if (auto vst2 = std::dynamic_pointer_cast<VSTNode>(eff)) {
            jEff["type"] = "VST2";
            jEff["path"] = vst2->getPath();
            jEff["name"] = vst2->getName();
        } else {
            jEff["type"] = eff->getName();
        }
        jEff["bypassed"] = eff->isBypassed();


        json jParams = json::array();
        for(int paramIdx=0; paramIdx<eff->getParameterCount(); ++paramIdx) {
            jParams.push_back(eff->getParameter(paramIdx));
        }
        jEff["params"] = jParams;


        std::vector<char> chunk;
        if (eff->getChunk(chunk)) {
            jEff["chunk"] = base64_encode(chunk);
        }
        return jEff;
    }

    void restoreEffectState(const json& jEff, const std::shared_ptr<EffectNode>& effect) {
        if (!effect) return;

        if (jEff.contains("bypassed")) {
            effect->setBypass(jEff["bypassed"]);
        }

        bool chunkRestored = false;
        if (jEff.contains("chunk")) {
            std::string encoded = jEff["chunk"];
            auto data = base64_decode(encoded);
            if (!data.empty()) {
                chunkRestored = effect->setChunk(data);
            }

            if (!chunkRestored) {
                std::cerr << "Warning: Failed to restore chunk for effect: " << jEff.value("type", "") << std::endl;
            }
        }

        if (!chunkRestored && jEff.contains("params")) {
            int idx = 0;
            for (float val : jEff["params"]) {
                effect->setParameter(idx++, val);
            }
        }
    }

    std::shared_ptr<EffectNode> deserializeEffect(const json& jEff) {
        std::string effType = jEff.value("type", "");
        std::shared_ptr<EffectNode> effect = nullptr;

        try {
            if (effType == "Delay") effect = std::make_shared<DelayNode>();
            else if (effType == "EQ3") effect = std::make_shared<EQ3Node>();
            else if (effType == "Reverb") effect = std::make_shared<ReverbNode>();
            else if (effType == "Compressor") effect = std::make_shared<CompressorNode>();
            else if (effType == "Limiter") effect = std::make_shared<LimiterNode>();
            else if (effType == "Pulsar" || effType == "Synthesizer") effect = std::make_shared<SynthesizerNode>();
            else if (effType == "Comet" || effType == "Sampler") effect = std::make_shared<SamplerNode>();
            else if (effType == "Flux Shaper") effect = std::make_shared<FluxShaperNode>();
            else if (effType == "Prism Stack") effect = std::make_shared<PrismStackNode>();
            else if (effType == "VST3") {
                if (jEff.contains("path") && jEff.contains("cid")) {
                    effect = std::make_shared<VST3Node>(jEff["path"], jEff["cid"]);
                }
            }
            else if (effType == "VST2") {
                if (jEff.contains("path")) {
                    std::string p = jEff["path"];
                    effect = PluginManager::loadPlugin(p);
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "Error deserializing effect " << effType << ": " << e.what() << std::endl;
            return nullptr;
        }
        return effect;
    }
}

    bool ProjectSerializer::save(const AudioEngine& engine, const std::string& filepath, const std::string& extraData) {
        fs::path projectPath(filepath);
        fs::path projectDir = projectPath.parent_path();
        if (projectDir.empty()) projectDir = fs::current_path();
        fs::path samplesDir = projectDir / "samples";

        try {
            if (!fs::exists(samplesDir)) fs::create_directories(samplesDir);
        } catch (...) {}

        json j;
        j["version"] = "1.1.0";
        j["sampleRate"] = engine.getSampleRate();
        j["looping"] = engine.isLooping();
        j["loopStart"] = engine.getLoopStart();
        j["loopEnd"] = engine.getLoopEnd();
        j["bpm"] = engine.getBpm();
        j["author"] = engine.getAuthor();
        j["genre"] = engine.getGenre();
        j["metronomeEnabled"] = engine.isMetronomeEnabled();
        j["metronomeVolume"] = engine.getMetronomeVolume();


        json jTempo = json::array();
        for(const auto& m : engine.getTempoMarkers()) jTempo.push_back({{"pos", m.position}, {"bpm", m.bpm}});
        j["tempoMarkers"] = jTempo;

        json jTimeSig = json::array();
        for(const auto& m : engine.getTimeSigMarkers()) jTimeSig.push_back({{"pos", m.position}, {"num", m.numerator}, {"den", m.denominator}});
        j["timeSigMarkers"] = jTimeSig;

        json jRegion = json::array();
        for(const auto& m : engine.getRegionMarkers()) jRegion.push_back({{"pos", m.position}, {"name", m.name}});
        j["regionMarkers"] = jRegion;


        if (auto master = engine.getMasterNode()) {
            j["masterVolume"] = master->getVolume();
            json jMasterFX = json::array();
            if (auto effects = master->getEffects()) {
                for(const auto& eff : *effects) {
                    jMasterFX.push_back(serializeEffect(eff));
                }
            }
            j["masterEffects"] = jMasterFX;
        }


        if (!extraData.empty()) {
            try {
                j["extra"] = json::parse(extraData);
            } catch(...) {
                j["extra"] = extraData;
            }
        }

        const auto& tracks = engine.getTracks();


        json jTracks = json::array();
        for (size_t i = 0; i < tracks.size(); ++i) {
            auto track = tracks[i];
            json jTrack;
            jTrack["name"] = track->getName();
            jTrack["volume"] = track->getVolume();
            jTrack["pan"] = track->getPan();
            jTrack["mute"] = track->isMuted();
            jTrack["solo"] = track->isSolo();
            jTrack["soloSafe"] = track->isSoloSafe();
            jTrack["phaseInverted"] = track->isPhaseInverted();
            jTrack["forceMono"] = track->isForceMono();
            jTrack["inputChannel"] = track->getInputChannel();

            int outIdx = -1;
            auto target = track->getOutputTarget();
            if (target) {
                for (size_t k = 0; k < tracks.size(); ++k) {
                    if (tracks[k] == target) {
                        outIdx = (int)k;
                        break;
                    }
                }
            }
            jTrack["outputTargetIndex"] = outIdx;

            const float* c = track->getColor();
            jTrack["color"] = { c[0], c[1], c[2] };


            auto instTrack = std::dynamic_pointer_cast<InstrumentTrack>(track);
            if (instTrack) {
                jTrack["type"] = "instrument";

                auto instrument = instTrack->getInstrument();
                if (auto effInst = std::dynamic_pointer_cast<EffectNode>(instrument)) {
                    jTrack["instrument"] = serializeEffect(effInst);
                }


                json jMidiClips = json::array();
                for (const auto& clip : *instTrack->getMidiClips()) {
                    json jClip;
                    jClip["startFrame"] = clip->getStartFrame();
                    jClip["name"] = clip->getName();
                    jClip["lengthFrames"] = clip->getLengthFrames();
                    jClip["offsetFrames"] = clip->getOffsetFrames();
                    const float* cc = clip->getColor();
                    jClip["color"] = { cc[0], cc[1], cc[2] };


                    json jNotes = json::array();
                    for(const auto& note : clip->getNotes()) {
                        json jNote;
                        jNote["start"] = note.startFrame;
                        jNote["len"] = note.lengthFrames;
                        jNote["note"] = note.noteNumber;
                        jNote["vel"] = note.velocity;
                        jNotes.push_back(jNote);
                    }
                    jClip["notes"] = jNotes;
                    jMidiClips.push_back(jClip);
                }
                jTrack["midiClips"] = jMidiClips;

            } else {
                if (track->isSoloSafe()) jTrack["type"] = "aux";
                else jTrack["type"] = "audio";


                json jClips = json::array();
                if (track->getClips()) {
                    for (const auto& clip : *track->getClips()) {
                        json jClip;


                        std::string srcPath = clip->getFilePath();
                        bool copied = false;
                        if (!srcPath.empty() && fs::exists(srcPath)) {
                            try {
                                fs::path p(srcPath);
                                std::string filename = p.filename().string();
                                fs::path dest = samplesDir / filename;


                                if (fs::exists(dest)) {
                                    if (!fs::equivalent(p, dest)) {
                                        std::string stem = p.stem().string();
                                        std::string ext = p.extension().string();
                                        int counter = 1;

                                        while (fs::exists(dest)) {
                                            if (fs::equivalent(p, dest)) {
                                                break;
                                            }
                                            dest = samplesDir / (stem + "_" + std::to_string(counter++) + ext);
                                        }
                                    }
                                }

                                if (!fs::exists(dest)) {
                                    fs::copy_file(p, dest);
                                }


                                std::string relPath = "samples/" + dest.filename().string();
                                jClip["path"] = relPath;
                                copied = true;
                            } catch (...) {}
                        }
                        if (!copied) jClip["path"] = srcPath;

                        jClip["startFrame"] = clip->getStartFrame();
                        jClip["lengthFrames"] = clip->getLengthFrames();
                        jClip["sourceStartFrame"] = clip->getSourceStartFrame();
                        jClip["name"] = clip->getName();
                        jClip["gain"] = clip->getGain();
                        jClip["pitch"] = clip->getPitch();
                        jClip["preservePitch"] = clip->isPreservePitch();
                        jClip["fadeIn"] = clip->getFadeIn();
                        jClip["fadeOut"] = clip->getFadeOut();
                        jClip["loop"] = clip->isLooping();
                        const float* cc = clip->getColor();
                        jClip["color"] = { cc[0], cc[1], cc[2] };

                        jClips.push_back(jClip);
                    }
                }
                jTrack["clips"] = jClips;
            }


            json jAuto;
            if (auto vol = track->getVolumeAutomation()) {
                json jVol = json::array();
                for (const auto& p : vol->getPoints()) {
                    jVol.push_back({ {"frame", p.frame}, {"value", p.value}, {"curve", p.curvature} });
                }
                jAuto["volume"] = jVol;
            }
            if (auto pan = track->getPanAutomation()) {
                json jPan = json::array();
                for (const auto& p : pan->getPoints()) {
                    jPan.push_back({ {"frame", p.frame}, {"value", p.value}, {"curve", p.curvature} });
                }
                jAuto["pan"] = jPan;
            }
            if (!jAuto.is_null()) jTrack["automation"] = jAuto;


            json jEffects = json::array();
            for (const auto& eff : *track->getEffects()) {
                jEffects.push_back(serializeEffect(eff));
            }
            jTrack["effects"] = jEffects;


            json jSends = json::array();
            for (const auto& send : track->getSends()) {
                json jSend;
                int targetIdx = -1;
                for (size_t k = 0; k < tracks.size(); ++k) {
                    if (tracks[k] == send.target) {
                        targetIdx = (int)k;
                        break;
                    }
                }
                if (targetIdx != -1 && send.gainNode) {
                    jSend["targetIndex"] = targetIdx;
                    jSend["gain"] = send.gainNode->getGain();
                    jSend["preFader"] = send.preFader;
                    jSends.push_back(jSend);
                }
            }
            jTrack["sends"] = jSends;

            jTracks.push_back(jTrack);
        }
        j["tracks"] = jTracks;

        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        file << j.dump(4);
        return true;
    }

    bool ProjectSerializer::load(AudioEngine& engine, const std::string& filepath, std::string* extraData) {
        fs::path projectPath(filepath);
        fs::path projectDir = projectPath.parent_path();
        if (projectDir.empty()) projectDir = fs::current_path();

        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        json j;
        try {
            file >> j;
        } catch (json::parse_error& e) {
            std::cerr << "JSON Parse Error: " << e.what() << std::endl;
            return false;
        }

        std::string projectVersion = j.value("version", "0.0.0");

        engine.stop();
        engine.panic();
        engine.clearMarkers();
        if (auto master = engine.getMasterNode()) {
             master->clearEffects();
        }
        engine.clearTracks();
        engine.suspendGraphRebuild();


        if (extraData && j.contains("extra")) {
            if (j["extra"].is_string()) *extraData = j["extra"];
            else *extraData = j["extra"].dump();
        }

        if (j.contains("looping")) engine.setLooping(j["looping"]);
        if (j.contains("loopStart") && j.contains("loopEnd")) engine.setLoopPoints(j["loopStart"], j["loopEnd"]);
        if (j.contains("bpm")) engine.setBpm(j["bpm"]);
        if (j.contains("author")) engine.setAuthor(j["author"]);
        if (j.contains("genre")) engine.setGenre(j["genre"]);
        if (j.contains("metronomeEnabled")) engine.setMetronomeEnabled(j["metronomeEnabled"]);
        if (j.contains("metronomeVolume")) engine.setMetronomeVolume(j["metronomeVolume"]);


        if (j.contains("tempoMarkers")) {
            for(const auto& m : j["tempoMarkers"]) engine.addTempoMarker(m["pos"], m["bpm"]);
        }
        if (j.contains("timeSigMarkers")) {
            for(const auto& m : j["timeSigMarkers"]) engine.addTimeSigMarker(m["pos"], m["num"], m["den"]);
        }
        if (j.contains("regionMarkers")) {
            for(const auto& m : j["regionMarkers"]) engine.addRegionMarker(m["pos"], m["name"]);
        }


        if (auto master = engine.getMasterNode()) {
            if (j.contains("masterVolume")) master->setVolume(j["masterVolume"]);
            if (j.contains("masterEffects")) {
                for(const auto& jEff : j["masterEffects"]) {
                    if (auto eff = deserializeEffect(jEff)) {
                        master->addEffect(eff);
                        restoreEffectState(jEff, eff);
                    }
                }
            }
        }

        std::vector<std::shared_ptr<AudioTrack>> createdTracks;

        if (j.contains("tracks")) {
            for (const auto& jTrack : j["tracks"]) {
                try {
                    std::shared_ptr<AudioTrack> track;
                    std::string type = jTrack.value("type", "audio");

                    if (type == "instrument") {
                    auto instTrack = engine.createInstrumentTrack();
                    track = instTrack;

                    bool externalInstrumentLoaded = false;


                    if (jTrack.contains("instrument")) {
                        if (auto inst = deserializeEffect(jTrack["instrument"])) {
                            instTrack->setInstrument(inst);
                            restoreEffectState(jTrack["instrument"], inst);
                            externalInstrumentLoaded = true;
                        }
                    } else if (jTrack.contains("instrumentPlugin")) {
                        if (auto inst = deserializeEffect(jTrack["instrumentPlugin"])) {
                            instTrack->setInstrument(inst);
                            restoreEffectState(jTrack["instrumentPlugin"], inst);
                            externalInstrumentLoaded = true;
                        }
                    }

                    // For backward compatibility
                    if (jTrack.contains("synth") && !externalInstrumentLoaded) {
                        auto jSynth = jTrack["synth"];
                        auto synth = instTrack->getSynthesizerNode();
                        if (synth) {
                            if (jSynth.contains("waveform")) synth->setWaveform((Waveform)jSynth["waveform"]);
                            if (jSynth.contains("cutoff")) synth->setFilterCutoff(jSynth["cutoff"]);
                            if (jSynth.contains("resonance")) synth->setFilterResonance(jSynth["resonance"]);
                        }
                    }

                    if (jTrack.contains("midiClips")) {
                        for (const auto& jClip : jTrack["midiClips"]) {
                            auto clip = std::make_shared<MidiClip>();
                            if (jClip.contains("startFrame")) clip->setStartFrame(jClip["startFrame"]);
                            if (jClip.contains("name")) clip->setName(jClip["name"]);
                            if (jClip.contains("lengthFrames")) clip->setLengthFrames(jClip["lengthFrames"]);
                            if (jClip.contains("offsetFrames")) clip->setOffsetFrames(jClip["offsetFrames"]);
                            if (jClip.contains("color")) {
                                auto c = jClip["color"];
                                clip->setColor(c[0], c[1], c[2]);
                            }

                            if (jClip.contains("notes")) {
                                for(const auto& jNote : jClip["notes"]) {
                                    MidiNote n;
                                    n.startFrame = jNote["start"];
                                    n.lengthFrames = jNote["len"];
                                    n.noteNumber = jNote["note"];
                                    n.velocity = jNote["vel"];
                                    clip->addNote(n);
                                }
                            }
                            instTrack->addMidiClip(clip);
                        }
                    }


                } else {
                    if (type == "aux") track = engine.createAuxTrack();
                    else track = engine.createTrack();

                    if (jTrack.contains("clips")) {
                        for (const auto& jClip : jTrack["clips"]) {
                            std::string path = jClip["path"];

                            if (!fs::exists(path) && !fs::path(path).is_absolute()) {
                                fs::path rel = projectDir / path;
                                if (fs::exists(rel)) path = rel.string();
                            }

                            auto clip = std::make_shared<AudioClip>();
                            if (clip->loadFromFile(path, (uint32_t)engine.getSampleRate())) {
                                if (jClip.contains("startFrame")) clip->setStartFrame(jClip["startFrame"]);
                                if (jClip.contains("lengthFrames")) clip->setLengthFrames(jClip["lengthFrames"]);
                                if (jClip.contains("sourceStartFrame")) clip->setSourceStartFrame(jClip["sourceStartFrame"]);
                                if (jClip.contains("name")) clip->setName(jClip["name"]);
                                if (jClip.contains("gain")) clip->setGain(jClip["gain"]);
                                if (jClip.contains("pitch")) clip->setPitch(jClip["pitch"]);
                                if (jClip.contains("preservePitch")) clip->setPreservePitch(jClip["preservePitch"]);
                                if (jClip.contains("fadeIn")) clip->setFadeIn(jClip["fadeIn"]);
                                if (jClip.contains("fadeOut")) clip->setFadeOut(jClip["fadeOut"]);
                                if (jClip.contains("loop")) clip->setLooping(jClip["loop"]);
                                if (jClip.contains("color")) {
                                    auto c = jClip["color"];
                                    clip->setColor(c[0], c[1], c[2]);
                                }

                                track->addClip(clip);
                            }
                        }
                    }
                }

                if (jTrack.contains("name")) track->setName(jTrack["name"]);
                if (jTrack.contains("volume")) track->setVolume(jTrack["volume"]);
                if (jTrack.contains("pan")) track->setPan(jTrack["pan"]);
                if (jTrack.contains("mute")) track->setMute(jTrack["mute"]);
                if (jTrack.contains("solo")) track->setSolo(jTrack["solo"]);
                if (jTrack.contains("soloSafe")) track->setSoloSafe(jTrack["soloSafe"]);
                if (jTrack.contains("phaseInverted")) track->setPhaseInverted(jTrack["phaseInverted"]);
                if (jTrack.contains("forceMono")) track->setForceMono(jTrack["forceMono"]);
                if (jTrack.contains("inputChannel")) track->setInputChannel(jTrack["inputChannel"]);
                if (jTrack.contains("color")) {
                    auto c = jTrack["color"];
                    track->setColor(c[0], c[1], c[2]);
                }


                if (jTrack.contains("automation")) {
                    auto jAuto = jTrack["automation"];
                    if (jAuto.contains("volume")) {
                        auto lane = std::make_shared<AutomationLane>();
                        for (const auto& p : jAuto["volume"]) {
                            lane->addPoint(p["frame"], p["value"], p.value("curve", 0.0f));
                        }
                        track->setVolumeAutomation(lane);
                    }
                    if (jAuto.contains("pan")) {
                        auto lane = std::make_shared<AutomationLane>();
                        for (const auto& p : jAuto["pan"]) {
                            lane->addPoint(p["frame"], p["value"], p.value("curve", 0.0f));
                        }
                        track->setPanAutomation(lane);
                    }
                }

                createdTracks.push_back(track);


                if (jTrack.contains("effects")) {
                    for (const auto& jEff : jTrack["effects"]) {
                        std::shared_ptr<EffectNode> effect = deserializeEffect(jEff);

                        if (effect) {



                             bool promoted = false;
                             if (auto instTrack = std::dynamic_pointer_cast<InstrumentTrack>(track)) {



                                 bool isDefaultSynth = (instTrack->getSynthesizerNode() != nullptr);


                                 if (isVersionOlderThan(projectVersion, 1, 1, 0) && isDefaultSynth && effect->isInstrument()) {
                                     instTrack->setInstrument(effect);
                                     restoreEffectState(jEff, effect);
                                     promoted = true;
                                 }
                             }

                             if (!promoted) {
                                track->addEffect(effect);
                                restoreEffectState(jEff, effect);
                             }
                        }
                    }
                }
                } catch (const std::exception& e) {
                    std::cerr << "Error loading track: " << e.what() << std::endl;
                }
            }
        }


        if (j.contains("tracks")) {
            size_t trackIdx = 0;
            for (const auto& jTrack : j["tracks"]) {
                if (trackIdx >= createdTracks.size()) break;
                auto track = createdTracks[trackIdx];

                if (jTrack.contains("outputTargetIndex")) {
                    int outIdx = jTrack["outputTargetIndex"];
                    if (outIdx >= 0 && outIdx < (int)createdTracks.size()) {
                        engine.routeTrackTo(track, createdTracks[outIdx]);
                    }
                }

                if (jTrack.contains("sends")) {
                    for (const auto& jSend : jTrack["sends"]) {
                        int targetIdx = jSend["targetIndex"];
                        float gain = jSend["gain"];
                        bool preFader = jSend.value("preFader", false);

                        if (targetIdx >= 0 && targetIdx < (int)createdTracks.size()) {
                            auto target = createdTracks[targetIdx];
                            track->addSend(target, preFader);
                            auto sends = track->getSends();
                            if (!sends.empty()) {
                                if (sends.back().gainNode) {
                                    sends.back().gainNode->setGain(gain);
                                }
                            }
                        }
                    }
                }
                trackIdx++;
            }
        }

        engine.resumeGraphRebuild();
        engine.updateSoloState();

        return true;
    }

}
