#include "ExtensionManager.h"
#include "orion/SettingsManager.h"
#include "orion/Logger.h"
#include "orion/AudioEngine.h"
#include "orion/InstrumentTrack.h"
#include "orion/MidiClip.h"
#include "ProjectManager.h"
#include "ExtensionApiIndex.h"
#ifndef ORION_TEST_HEADLESS
#include <juce_gui_basics/juce_gui_basics.h>
#endif
#include <filesystem>
#include <fstream>
#include <cmath>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace Orion {

    ExtensionManager& ExtensionManager::getInstance() {
        static ExtensionManager instance;
        return instance;
    }

    void ExtensionManager::setContext(AudioEngine* e, ProjectManager* pm) {
        engine = e;
        projectManager = pm;
    }

    void ExtensionManager::initialize() {
        if (L) return;

        L = luaL_newstate();
        luaL_openlibs(L);

        bindAPI();

        installFactoryExtensions();

        log("Lua Engine Initialized.");
        scanExtensions();
    }

    void ExtensionManager::shutdown() {
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }



    static double beatsToSeconds(double beats, float bpm) {
        if (bpm <= 0.0f) return 0.0;
        return beats * (60.0 / bpm);
    }

    static double secondsToBeats(double seconds, float bpm) {
        if (bpm <= 0.0f) return 0.0;
        return seconds * (bpm / 60.0);
    }

    static uint64_t beatsToFrames(double beats, float bpm, double sampleRate) {
        return (uint64_t)(beatsToSeconds(beats, bpm) * sampleRate);
    }




    static int lua_orion_log(lua_State* L) {
        const char* msg = lua_tostring(L, 1);
        int levelInt = (int)luaL_optinteger(L, 2, 0); // 0: Info, 1: Warning, 2: Error, 3: Success
        LogLevel level = LogLevel::Info;
        if (levelInt == 1) level = LogLevel::Warning;
        else if (levelInt == 2) level = LogLevel::Error;
        else if (levelInt == 3) level = LogLevel::Success;

        if (msg) ExtensionManager::getInstance().log(msg, level);
        return 0;
    }

    static int lua_orion_alert(lua_State* L) {
        const char* title = lua_tostring(L, 1);
        const char* msg = lua_tostring(L, 2);
#ifndef ORION_TEST_HEADLESS
        if (title && msg) {
            std::string t = title;
            std::string m = msg;
            juce::MessageManager::callAsync([t, m] {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, t, m);
            });
        }
#else
        if (title && msg) {
            ExtensionManager::getInstance().log("Alert (Headless): " + std::string(title) + " - " + std::string(msg));
        }
#endif
        return 0;
    }

    static int lua_orion_getVersion(lua_State* L) {
        lua_pushstring(L, "0.1.0");
        return 1;
    }


    static int lua_transport_play(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->play();
        return 0;
    }
    static int lua_transport_stop(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->stop();
        return 0;
    }
    static int lua_transport_pause(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->pause();
        return 0;
    }
    static int lua_transport_record(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->record();
        return 0;
    }
    static int lua_transport_panic(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->panic();
        return 0;
    }
    static int lua_transport_isPlaying(lua_State* L) {
        bool playing = false;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            playing = (e->getTransportState() == TransportState::Playing || e->getTransportState() == TransportState::Recording);
        lua_pushboolean(L, playing);
        return 1;
    }
    static int lua_transport_isPaused(lua_State* L) {
        bool paused = false;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            paused = (e->getTransportState() == TransportState::Paused);
        lua_pushboolean(L, paused);
        return 1;
    }
    static int lua_transport_isRecording(lua_State* L) {
        bool rec = false;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            rec = (e->getTransportState() == TransportState::Recording);
        lua_pushboolean(L, rec);
        return 1;
    }
    static int lua_transport_getPosition(lua_State* L) {
        double pos = 0.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            if (e->getSampleRate() > 0)
                pos = (double)e->getCursor() / e->getSampleRate();
        }
        lua_pushnumber(L, pos);
        return 1;
    }
    static int lua_transport_setPosition(lua_State* L) {
        double pos = lua_tonumber(L, 1);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            e->setCursor((uint64_t)(pos * e->getSampleRate()));
        }
        return 0;
    }
    static int lua_transport_getPositionBeats(lua_State* L) {
        double beats = 0.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            double secs = (e->getSampleRate() > 0) ? (double)e->getCursor() / e->getSampleRate() : 0.0;
            beats = secondsToBeats(secs, e->getBpm());
        }
        lua_pushnumber(L, beats);
        return 1;
    }
    static int lua_transport_setPositionBeats(lua_State* L) {
        double beats = lua_tonumber(L, 1);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            double secs = beatsToSeconds(beats, e->getBpm());
            e->setCursor((uint64_t)(secs * e->getSampleRate()));
        }
        return 0;
    }
    static int lua_transport_isLooping(lua_State* L) {
        bool looping = false;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) looping = e->isLooping();
        lua_pushboolean(L, looping);
        return 1;
    }
    static int lua_transport_setLooping(lua_State* L) {
        bool looping = lua_toboolean(L, 1) != 0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->setLooping(looping);
        return 0;
    }
    static int lua_transport_getLoopStart(lua_State* L) {
        double secs = 0.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            if (e->getSampleRate() > 0) secs = (double)e->getLoopStart() / e->getSampleRate();
        }
        lua_pushnumber(L, secs);
        return 1;
    }
    static int lua_transport_getLoopEnd(lua_State* L) {
        double secs = 0.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            if (e->getSampleRate() > 0) secs = (double)e->getLoopEnd() / e->getSampleRate();
        }
        lua_pushnumber(L, secs);
        return 1;
    }
    static int lua_transport_setLoopPoints(lua_State* L) {
        double startSec = lua_tonumber(L, 1);
        double endSec   = lua_tonumber(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            double sr = e->getSampleRate();
            e->setLoopPoints((uint64_t)(startSec * sr), (uint64_t)(endSec * sr));
        }
        return 0;
    }


    static int lua_project_getBpm(lua_State* L) {
        float bpm = 120.0f;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) bpm = e->getBpm();
        lua_pushnumber(L, bpm);
        return 1;
    }
    static int lua_project_setBpm(lua_State* L) {
        float bpm = (float)lua_tonumber(L, 1);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->setBpm(bpm);
        return 0;
    }
    static int lua_project_getAuthor(lua_State* L) {
        std::string s;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) s = e->getAuthor();
        lua_pushstring(L, s.c_str());
        return 1;
    }
    static int lua_project_setAuthor(lua_State* L) {
        const char* s = lua_tostring(L, 1);
        if (s) if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->setAuthor(s);
        return 0;
    }
    static int lua_project_getGenre(lua_State* L) {
        std::string s;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) s = e->getGenre();
        lua_pushstring(L, s.c_str());
        return 1;
    }
    static int lua_project_setGenre(lua_State* L) {
        const char* s = lua_tostring(L, 1);
        if (s) if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->setGenre(s);
        return 0;
    }
    static int lua_project_getKeySignature(lua_State* L) {
        std::string s = "C Maj";
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) s = e->getKeySignature();
        lua_pushstring(L, s.c_str());
        return 1;
    }
    static int lua_project_setKeySignature(lua_State* L) {
        const char* s = lua_tostring(L, 1);
        if (s) if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->setKeySignature(s);
        return 0;
    }
    static int lua_project_getTimeSignature(lua_State* L) {
        int num = 4, den = 4;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->getTimeSignature(num, den);
        lua_pushinteger(L, num);
        lua_pushinteger(L, den);
        return 2;
    }
    static int lua_project_setTimeSignature(lua_State* L) {
        int num = (int)lua_tointeger(L, 1);
        int den = (int)lua_tointeger(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->setTimeSignature(num, den);
        return 0;
    }
    static int lua_project_getSampleRate(lua_State* L) {
        double sr = 44100.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) sr = e->getSampleRate();
        lua_pushnumber(L, sr);
        return 1;
    }
    static int lua_project_clearAllTracks(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->clearTracks();
        return 0;
    }
    static int lua_project_timeToBeats(lua_State* L) {
        double seconds = lua_tonumber(L, 1);
        double beats = 0.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            beats = secondsToBeats(seconds, e->getBpm());
        }
        lua_pushnumber(L, beats);
        return 1;
    }
    static int lua_project_beatsToTime(lua_State* L) {
        double beats = lua_tonumber(L, 1);
        double seconds = 0.0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            seconds = beatsToSeconds(beats, e->getBpm());
        }
        lua_pushnumber(L, seconds);
        return 1;
    }


    static int lua_track_getCount(lua_State* L) {
        int count = 0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) count = (int)e->getTracks().size();
        lua_pushinteger(L, count);
        return 1;
    }
    static int lua_track_getName(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        std::string name;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                name = tracks[idx]->getName();
            }
        }
        lua_pushstring(L, name.c_str());
        return 1;
    }
    static int lua_track_addAudioTrack(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->createTrack();
        return 0;
    }
    static int lua_track_addInstrumentTrack(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) e->createInstrumentTrack();
        return 0;
    }
    static int lua_track_delete(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                e->removeTrack(tracks[idx]);
            }
        }
        return 0;
    }
    static int lua_track_rename(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        const char* name = lua_tostring(L, 2);
        if (name) {
            if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
                const auto& tracks = e->getTracks();
                if (idx >= 0 && idx < (int)tracks.size()) {
                    tracks[idx]->setName(name);
                }
            }
        }
        return 0;
    }



    static int lua_track_createMidiClip(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        const char* name = lua_tostring(L, 2);
        double startBeat = lua_tonumber(L, 3);
        double lengthBeat = lua_tonumber(L, 4);

        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                if (auto* instTrack = dynamic_cast<InstrumentTrack*>(tracks[idx].get())) {
                    auto clip = std::make_shared<MidiClip>();
                    if (name) clip->setName(name);

                    uint64_t startFrame = beatsToFrames(startBeat, e->getBpm(), e->getSampleRate());
                    uint64_t lenFrame = beatsToFrames(lengthBeat, e->getBpm(), e->getSampleRate());

                    clip->setStartFrame(startFrame);
                    clip->setLengthFrames(lenFrame);

                    instTrack->addMidiClip(clip);
                } else {
                    ExtensionManager::getInstance().log("Error: Track " + std::to_string(idx + 1) + " is not an Instrument Track.");
                }
            }
        }
        return 0;
    }


    static int lua_track_getMidiClipCount(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        int count = 0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                if (auto* instTrack = dynamic_cast<InstrumentTrack*>(tracks[idx].get())) {
                    if (auto clips = instTrack->getMidiClips()) {
                        count = (int)clips->size();
                    }
                }
            }
        }
        lua_pushinteger(L, count);
        return 1;
    }


    static int lua_track_getAudioClipCount(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        int count = 0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                 if (auto clips = tracks[idx]->getClips()) {
                     count = (int)clips->size();
                 }
            }
        }
        lua_pushinteger(L, count);
        return 1;
    }



    static int lua_clip_addNote(lua_State* L) {
        int trackIdx = (int)lua_tointeger(L, 1) - 1;
        int clipIdx = (int)lua_tointeger(L, 2) - 1;
        int noteNum = (int)lua_tointeger(L, 3);
        float velocity = (float)lua_tonumber(L, 4);
        double startBeat = lua_tonumber(L, 5);
        double durBeat = lua_tonumber(L, 6);

        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
                if (auto* instTrack = dynamic_cast<InstrumentTrack*>(tracks[trackIdx].get())) {
                    if (auto clips = instTrack->getMidiClips()) {
                        if (clipIdx >= 0 && clipIdx < (int)clips->size()) {
                            auto clip = (*clips)[clipIdx];








                            uint64_t startFrame = beatsToFrames(startBeat, e->getBpm(), e->getSampleRate());
                            uint64_t lenFrame = beatsToFrames(durBeat, e->getBpm(), e->getSampleRate());

                            MidiNote note;
                            note.noteNumber = noteNum;
                            note.velocity = velocity;
                            note.startFrame = startFrame;
                            note.lengthFrames = lenFrame;

                            clip->addNote(note);
                            clip->sortNotes();
                        }
                    }
                }
            }
        }
        return 0;
    }


    static int lua_mixer_setVolume(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        float vol = (float)lua_tonumber(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                tracks[idx]->setVolume(vol);
            }
        }
        return 0;
    }
    static int lua_mixer_getVolume(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        float vol = 0.0f;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                vol = tracks[idx]->getVolume();
            }
        }
        lua_pushnumber(L, vol);
        return 1;
    }
    static int lua_mixer_setPan(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        float pan = (float)lua_tonumber(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                tracks[idx]->setPan(pan);
            }
        }
        return 0;
    }
    static int lua_mixer_getPan(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        float pan = 0.0f;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                pan = tracks[idx]->getPan();
            }
        }
        lua_pushnumber(L, pan);
        return 1;
    }
    static int lua_mixer_setMute(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        bool mute = lua_toboolean(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                tracks[idx]->setMute(mute);
            }
        }
        return 0;
    }
    static int lua_mixer_setSolo(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        bool solo = lua_toboolean(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                tracks[idx]->setSolo(solo);
                e->updateSoloState();
            }
        }
        return 0;
    }


    static int lua_ui_confirm(lua_State* L) {
        const char* title = lua_tostring(L, 1);
        const char* msg = lua_tostring(L, 2);

        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);

#ifndef ORION_TEST_HEADLESS
            juce::MessageManager::callAsync([titleStr = std::string(title ? title : ""), msgStr = std::string(msg ? msg : ""), cbRef, L]() {
                juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon, titleStr, msgStr, "Yes", "No", nullptr,
                    juce::ModalCallbackFunction::create([cbRef, L](int result) {
                        bool confirmed = (result == 1);
                        lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef);
                        lua_pushboolean(L, confirmed);
                        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                            ExtensionManager::getInstance().log("Error in confirm callback: " + std::string(lua_tostring(L, -1)));
                            lua_pop(L, 1);
                        }
                        luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
                    })
                );
            });
#else
            juce::ignoreUnused(title, msg);
            ExtensionManager::getInstance().log("ui.confirm ignored (headless)");
            luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
#endif
            return 0;
        } else {
            ExtensionManager::getInstance().log("Warning: ui.confirm requires a callback function (async).");
            return 0;
        }
    }

    static int lua_ui_input(lua_State* L) {
        const char* title = lua_tostring(L, 1);
        const char* msg = lua_tostring(L, 2);

        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);

#ifndef ORION_TEST_HEADLESS
            juce::MessageManager::callAsync([titleStr = std::string(title ? title : ""), msgStr = std::string(msg ? msg : ""), cbRef, L]() {
                auto* w = new juce::AlertWindow(titleStr, msgStr, juce::AlertWindow::QuestionIcon);
                w->addTextEditor("input", "", "Value:");
                w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

                w->enterModalState(true, juce::ModalCallbackFunction::create([w, cbRef, L](int result) {
                    std::string text = w->getTextEditorContents("input").toStdString();
                    bool ok = (result == 1);

                    lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef);
                    if (ok) lua_pushstring(L, text.c_str());
                    else lua_pushnil(L);

                    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                         ExtensionManager::getInstance().log("Error in input callback: " + std::string(lua_tostring(L, -1)));
                         lua_pop(L, 1);
                    }
                    luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
                    delete w;
                }));
            });
#else
            juce::ignoreUnused(title, msg);
            ExtensionManager::getInstance().log("ui.input ignored (headless)");
            luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
#endif
            return 0;
        } else {
            ExtensionManager::getInstance().log("Warning: ui.input requires a callback function (async).");
            return 0;
        }
    }

    static int lua_ui_getSelectedClips(lua_State* L) {
        lua_newtable(L);
        if (ExtensionManager::getInstance().getSelectedClips) {
            auto items = ExtensionManager::getInstance().getSelectedClips();
            for (size_t i = 0; i < items.size(); ++i) {
                lua_newtable(L);
                lua_pushinteger(L, items[i].trackIndex); lua_setfield(L, -2, "track");
                lua_pushinteger(L, items[i].clipIndex); lua_setfield(L, -2, "clip");
                lua_rawseti(L, -2, (int)i + 1);
            }
        }
        return 1;
    }

    static int lua_ui_getSelectedNotes(lua_State* L) {
        lua_newtable(L);
        if (ExtensionManager::getInstance().getSelectedNotes) {
            auto items = ExtensionManager::getInstance().getSelectedNotes();
            for (size_t i = 0; i < items.size(); ++i) {
                lua_newtable(L);
                lua_pushinteger(L, items[i].trackIndex); lua_setfield(L, -2, "track");
                lua_pushinteger(L, items[i].clipIndex); lua_setfield(L, -2, "clip");
                lua_pushinteger(L, items[i].noteIndex); lua_setfield(L, -2, "note");
                lua_rawseti(L, -2, (int)i + 1);
            }
        }
        return 1;
    }

    // --- Additional track functions ---
    static int lua_track_getType(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        const char* type = "audio";
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                if (dynamic_cast<InstrumentTrack*>(tracks[idx].get()))
                    type = "instrument";
            }
        }
        lua_pushstring(L, type);
        return 1;
    }

    static int lua_track_isMuted(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        bool muted = false;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size())
                muted = tracks[idx]->isMuted();
        }
        lua_pushboolean(L, muted);
        return 1;
    }

    static int lua_track_isSoloed(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        bool soloed = false;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size())
                soloed = tracks[idx]->isSolo();
        }
        lua_pushboolean(L, soloed);
        return 1;
    }

    static int lua_track_moveTrack(lua_State* L) {
        int from = (int)lua_tointeger(L, 1) - 1;
        int to   = (int)lua_tointeger(L, 2) - 1;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            e->moveTrack((size_t)from, (size_t)to);
        return 0;
    }

    static int lua_track_setColor(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        float r = (float)lua_tonumber(L, 2);
        float g = (float)lua_tonumber(L, 3);
        float b = (float)lua_tonumber(L, 4);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (idx >= 0 && idx < (int)tracks.size()) {
                tracks[idx]->setColor(r, g, b);
            }
        }
        return 0;
    }

    // --- Additional clip functions ---
    static int lua_clip_getNoteCount(lua_State* L) {
        int trackIdx = (int)lua_tointeger(L, 1) - 1;
        int clipIdx  = (int)lua_tointeger(L, 2) - 1;
        int count = 0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
                if (auto* it = dynamic_cast<InstrumentTrack*>(tracks[trackIdx].get())) {
                    if (auto clips = it->getMidiClips()) {
                        if (clipIdx >= 0 && clipIdx < (int)clips->size())
                            count = (int)(*clips)[clipIdx]->getNotes().size();
                    }
                }
            }
        }
        lua_pushinteger(L, count);
        return 1;
    }

    static int lua_clip_getNoteAt(lua_State* L) {
        int trackIdx = (int)lua_tointeger(L, 1) - 1;
        int clipIdx  = (int)lua_tointeger(L, 2) - 1;
        int noteIdx  = (int)lua_tointeger(L, 3) - 1;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
                if (auto* it = dynamic_cast<InstrumentTrack*>(tracks[trackIdx].get())) {
                    if (auto clips = it->getMidiClips()) {
                        if (clipIdx >= 0 && clipIdx < (int)clips->size()) {
                            const auto& notes = (*clips)[clipIdx]->getNotes();
                            if (noteIdx >= 0 && noteIdx < (int)notes.size()) {
                                const auto& n = notes[noteIdx];
                                double sr = e->getSampleRate() > 0 ? e->getSampleRate() : 44100.0;
                                float bpm = e->getBpm();
                                lua_newtable(L);
                                lua_pushinteger(L, n.noteNumber); lua_setfield(L, -2, "note");
                                lua_pushnumber(L, (double)n.velocity);   lua_setfield(L, -2, "velocity");
                                lua_pushnumber(L, (double)secondsToBeats((double)n.startFrame / sr, bpm)); lua_setfield(L, -2, "start");
                                lua_pushnumber(L, (double)secondsToBeats((double)n.lengthFrames / sr, bpm)); lua_setfield(L, -2, "duration");
                                return 1;
                            }
                        }
                    }
                }
            }
        }
        lua_pushnil(L);
        return 1;
    }

    static int lua_clip_clearNotes(lua_State* L) {
        int trackIdx = (int)lua_tointeger(L, 1) - 1;
        int clipIdx  = (int)lua_tointeger(L, 2) - 1;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
                if (auto* it = dynamic_cast<InstrumentTrack*>(tracks[trackIdx].get())) {
                    if (auto clips = it->getMidiClips()) {
                        if (clipIdx >= 0 && clipIdx < (int)clips->size())
                            (*clips)[clipIdx]->clearNotes();
                    }
                }
            }
        }
        return 0;
    }

    static int lua_clip_setClipName(lua_State* L) {
        int trackIdx = (int)lua_tointeger(L, 1) - 1;
        int clipIdx  = (int)lua_tointeger(L, 2) - 1;
        const char* name = lua_tostring(L, 3);
        if (name) {
            if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
                const auto& tracks = e->getTracks();
                if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
                    if (auto* it = dynamic_cast<InstrumentTrack*>(tracks[trackIdx].get())) {
                        if (auto clips = it->getMidiClips()) {
                            if (clipIdx >= 0 && clipIdx < (int)clips->size())
                                (*clips)[clipIdx]->setName(name);
                        }
                    }
                }
            }
        }
        return 0;
    }

    static int lua_clip_getClipName(lua_State* L) {
        int trackIdx = (int)lua_tointeger(L, 1) - 1;
        int clipIdx  = (int)lua_tointeger(L, 2) - 1;
        std::string name;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& tracks = e->getTracks();
            if (trackIdx >= 0 && trackIdx < (int)tracks.size()) {
                if (auto* it = dynamic_cast<InstrumentTrack*>(tracks[trackIdx].get())) {
                    if (auto clips = it->getMidiClips()) {
                        if (clipIdx >= 0 && clipIdx < (int)clips->size())
                            name = (*clips)[clipIdx]->getName();
                    }
                }
            }
        }
        lua_pushstring(L, name.c_str());
        return 1;
    }

    // --- Mixer: master volume ---
    static int lua_mixer_getMasterVolume(lua_State* L) {
        float vol = 1.0f;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            if (auto* master = e->getMasterNode())
                vol = master->getVolume();
        }
        lua_pushnumber(L, vol);
        return 1;
    }
    static int lua_mixer_setMasterVolume(lua_State* L) {
        float vol = (float)lua_tonumber(L, 1);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            if (auto* master = e->getMasterNode())
                master->setVolume(vol);
        }
        return 0;
    }
    static int lua_mixer_getMasterPan(lua_State* L) {
        lua_pushnumber(L, 0.0);
        return 1;
    }
    static int lua_mixer_setMasterPan(lua_State*) {
        return 0;
    }

    // --- Markers ---
    static int lua_markers_addRegion(lua_State* L) {
        double posSec = lua_tonumber(L, 1);
        const char* name = lua_tostring(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            double sr = e->getSampleRate() > 0 ? e->getSampleRate() : 44100.0;
            e->addRegionMarker((uint64_t)(posSec * sr), name ? name : "");
        }
        return 0;
    }
    static int lua_markers_getRegionCount(lua_State* L) {
        int count = 0;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            count = (int)e->getRegionMarkers().size();
        lua_pushinteger(L, count);
        return 1;
    }
    static int lua_markers_getRegion(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            const auto& markers = e->getRegionMarkers();
            if (idx >= 0 && idx < (int)markers.size()) {
                double sr = e->getSampleRate() > 0 ? e->getSampleRate() : 44100.0;
                lua_newtable(L);
                lua_pushnumber(L, (double)markers[idx].position / sr); lua_setfield(L, -2, "position");
                lua_pushstring(L, markers[idx].name.c_str());          lua_setfield(L, -2, "name");
                return 1;
            }
        }
        lua_pushnil(L);
        return 1;
    }
    static int lua_markers_removeRegion(lua_State* L) {
        int idx = (int)lua_tointeger(L, 1) - 1;
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            e->removeRegionMarker(idx);
        return 0;
    }
    static int lua_markers_addTempo(lua_State* L) {
        double posSec = lua_tonumber(L, 1);
        float bpm = (float)lua_tonumber(L, 2);
        if (auto* e = ExtensionManager::getInstance().getAudioEngine()) {
            double sr = e->getSampleRate() > 0 ? e->getSampleRate() : 44100.0;
            e->addTempoMarker((uint64_t)(posSec * sr), bpm);
        }
        return 0;
    }
    static int lua_markers_clearTempoMarkers(lua_State*) {
        if (auto* e = ExtensionManager::getInstance().getAudioEngine())
            e->clearMarkers();
        return 0;
    }

    // --- orion.getTheme ---
    static int lua_orion_getTheme(lua_State* L) {
        std::string theme = "Unknown";
        if (ExtensionManager::getInstance().getCurrentThemeName)
            theme = ExtensionManager::getInstance().getCurrentThemeName();
        lua_pushstring(L, theme.c_str());
        return 1;
    }


    void ExtensionManager::bindAPI() {
        if (!L) return;

        std::vector<juce::String> symbols;
        symbols.reserve(128);

        auto bindNamespaced = [this, &symbols](const char* ns, const char* name, lua_CFunction fn) {
            juce::ignoreUnused(this);
            lua_pushcfunction(L, fn);
            lua_setfield(L, -2, name);
            symbols.push_back("orion." + juce::String(ns) + "." + juce::String(name));
        };

        auto bindRoot = [this, &symbols](const char* name, lua_CFunction fn) {
            juce::ignoreUnused(this);
            lua_pushcfunction(L, fn);
            lua_setfield(L, -2, name);
            symbols.push_back("orion." + juce::String(name));
        };

        auto bindModule = [this, &bindNamespaced](const char* ns,
                                                  std::initializer_list<std::pair<const char*, lua_CFunction>> methods) {
            juce::ignoreUnused(this);
            lua_newtable(L);
            for (const auto& method : methods)
                bindNamespaced(ns, method.first, method.second);
            lua_setfield(L, -2, ns);
        };

        lua_newtable(L);  // the main "orion" table

        bindModule("transport", {
            {"play", lua_transport_play},
            {"stop", lua_transport_stop},
            {"pause", lua_transport_pause},
            {"record", lua_transport_record},
            {"panic", lua_transport_panic},
            {"isPlaying", lua_transport_isPlaying},
            {"isPaused", lua_transport_isPaused},
            {"isRecording", lua_transport_isRecording},
            {"getPosition", lua_transport_getPosition},
            {"setPosition", lua_transport_setPosition},
            {"getPositionBeats", lua_transport_getPositionBeats},
            {"setPositionBeats", lua_transport_setPositionBeats},
            {"isLooping", lua_transport_isLooping},
            {"setLooping", lua_transport_setLooping},
            {"getLoopStart", lua_transport_getLoopStart},
            {"getLoopEnd", lua_transport_getLoopEnd},
            {"setLoopPoints", lua_transport_setLoopPoints},
        });

        bindModule("project", {
            {"getBpm", lua_project_getBpm},
            {"setBpm", lua_project_setBpm},
            {"getAuthor", lua_project_getAuthor},
            {"setAuthor", lua_project_setAuthor},
            {"getGenre", lua_project_getGenre},
            {"setGenre", lua_project_setGenre},
            {"getKeySignature", lua_project_getKeySignature},
            {"setKeySignature", lua_project_setKeySignature},
            {"getTimeSignature", lua_project_getTimeSignature},
            {"setTimeSignature", lua_project_setTimeSignature},
            {"getSampleRate", lua_project_getSampleRate},
            {"clearAllTracks", lua_project_clearAllTracks},
            {"timeToBeats", lua_project_timeToBeats},
            {"beatsToTime", lua_project_beatsToTime},
        });

        bindModule("track", {
            {"getCount", lua_track_getCount},
            {"getName", lua_track_getName},
            {"getType", lua_track_getType},
            {"isMuted", lua_track_isMuted},
            {"isSoloed", lua_track_isSoloed},
            {"addAudioTrack", lua_track_addAudioTrack},
            {"addInstrumentTrack", lua_track_addInstrumentTrack},
            {"delete", lua_track_delete},
            {"rename", lua_track_rename},
            {"moveTrack", lua_track_moveTrack},
            {"setColor", lua_track_setColor},
            {"createMidiClip", lua_track_createMidiClip},
            {"getMidiClipCount", lua_track_getMidiClipCount},
            {"getAudioClipCount", lua_track_getAudioClipCount},
        });

        bindModule("clip", {
            {"addNote", lua_clip_addNote},
            {"getNoteCount", lua_clip_getNoteCount},
            {"getNoteAt", lua_clip_getNoteAt},
            {"clearNotes", lua_clip_clearNotes},
            {"setClipName", lua_clip_setClipName},
            {"getClipName", lua_clip_getClipName},
        });

        bindModule("mixer", {
            {"setVolume", lua_mixer_setVolume},
            {"getVolume", lua_mixer_getVolume},
            {"setPan", lua_mixer_setPan},
            {"getPan", lua_mixer_getPan},
            {"setMute", lua_mixer_setMute},
            {"setSolo", lua_mixer_setSolo},
            {"getMasterVolume", lua_mixer_getMasterVolume},
            {"setMasterVolume", lua_mixer_setMasterVolume},
            {"getMasterPan", lua_mixer_getMasterPan},
            {"setMasterPan", lua_mixer_setMasterPan},
        });

        bindModule("markers", {
            {"addRegion", lua_markers_addRegion},
            {"getRegionCount", lua_markers_getRegionCount},
            {"getRegion", lua_markers_getRegion},
            {"removeRegion", lua_markers_removeRegion},
            {"addTempo", lua_markers_addTempo},
            {"clearTempoMarkers", lua_markers_clearTempoMarkers},
        });

        bindModule("ui", {
            {"confirm", lua_ui_confirm},
            {"input", lua_ui_input},
            {"getSelectedClips", lua_ui_getSelectedClips},
            {"getSelectedNotes", lua_ui_getSelectedNotes},
        });

        bindRoot("log", lua_orion_log);
        bindRoot("alert", lua_orion_alert);
        bindRoot("getVersion", lua_orion_getVersion);
        bindRoot("getTheme", lua_orion_getTheme);

        lua_setglobal(L, "orion");

        ExtensionApiIndex::getInstance().rebuildFromBindings(symbols);
    }



    bool ExtensionManager::runScript(const std::string& scriptPath) {
        if (!L) initialize();

        if (luaL_dofile(L, scriptPath.c_str()) != LUA_OK) {
            std::string err = lua_tostring(L, -1);
            log("Error running script: " + err, LogLevel::Error);
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    bool ExtensionManager::runCode(const std::string& code) {
        if (!L) initialize();

        if (luaL_dostring(L, code.c_str()) != LUA_OK) {
            std::string err = lua_tostring(L, -1);
            log("Error running code: " + err, LogLevel::Error);
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    bool ExtensionManager::importExtension(const juce::File& oexFile) {
        if (!oexFile.existsAsFile()) return false;

        auto root = std::filesystem::path(SettingsManager::get().rootDataPath);
        if (root.empty()) root = std::filesystem::current_path();
        auto extDir = root / "Extensions";

        juce::ZipFile zip(oexFile);
        if (zip.getNumEntries() == 0) return false;

        auto result = zip.uncompressTo(juce::File(extDir.string()), true);
        if (result.failed()) {
            log("Failed to import extension: " + result.getErrorMessage().toStdString(), LogLevel::Error);
            return false;
        }

        log("Successfully imported extension: " + oexFile.getFileNameWithoutExtension().toStdString(), LogLevel::Success);
        scanExtensions();
        return true;
    }

    bool ExtensionManager::exportExtension(const std::string& name, const juce::File& targetFile) {
        auto root = std::filesystem::path(SettingsManager::get().rootDataPath);
        if (root.empty()) root = std::filesystem::current_path();
        auto extDir = root / "Extensions" / name;

        if (!std::filesystem::exists(extDir)) return false;

        if (targetFile.existsAsFile()) targetFile.deleteFile();

        juce::FileOutputStream fos(targetFile);
        if (fos.openedOk()) {
            juce::ZipFile::Builder builder;
            juce::File sourceDir(extDir.string());

            juce::Array<juce::File> files;
            sourceDir.findChildFiles(files, juce::File::findFiles, true);

            for (auto& f : files) {
                auto relativePath = f.getRelativePathFrom(sourceDir.getParentDirectory());
                builder.addFile(f, 9, relativePath);
            }

            builder.writeToStream(fos, nullptr);
            log("Successfully exported extension: " + name, LogLevel::Success);
            return true;
        }

        return false;
    }



    void ExtensionManager::log(const std::string& msg, LogLevel level) {
        LogMessage lm;
        lm.message = msg;
        lm.level = level;
        lm.timestamp = juce::Time::getCurrentTime().toString(true, true, true, true).toStdString();

        logHistory.push_back(lm);
        if (logHistory.size() > 1000) logHistory.erase(logHistory.begin());

        if (level == LogLevel::Error) Logger::error("[Lua] " + msg);
        else if (level == LogLevel::Warning) Logger::warn("[Lua] " + msg);
        else Logger::info("[Lua] " + msg);

        if (onLogMessage) onLogMessage(lm);
    }

    void ExtensionManager::scanExtensions() {
        extensions.clear();


        auto root = std::filesystem::path(SettingsManager::get().rootDataPath);
        if (root.empty()) root = std::filesystem::current_path();

        auto extDir = root / "Extensions";
        if (!std::filesystem::exists(extDir)) {
             std::filesystem::create_directories(extDir);


             auto exampleDir = extDir / "Example";
             std::filesystem::create_directories(exampleDir);

             std::ofstream f(exampleDir / "main.lua");
             f << "-- Example Extension for Orion\n";
             f << "orion.log('Hello from Lua!')\n";
             f << "orion.alert('Welcome', 'Extensions are enabled!')\n";
             f.close();
        }

        for (const auto& entry : std::filesystem::directory_iterator(extDir)) {
            if (entry.is_directory()) {
                auto mainLua = entry.path() / "main.lua";
                if (std::filesystem::exists(mainLua)) {
                    Extension ext;
                    ext.name = entry.path().filename().string();
                    ext.mainScript = mainLua.string();
                    ext.dir = entry.path().string();
                    ext.loaded = false;

                    // Try to read optional manifest.lua for metadata
                    auto manifestPath = entry.path() / "manifest.lua";
                    if (std::filesystem::exists(manifestPath) && L) {
                        // Run manifest in a sandboxed way using a temp Lua state
                        lua_State* mL = luaL_newstate();
                        if (mL) {
                            if (luaL_dofile(mL, manifestPath.string().c_str()) == LUA_OK) {
                                lua_getglobal(mL, "name");
                                if (lua_isstring(mL, -1)) ext.name = lua_tostring(mL, -1);
                                lua_pop(mL, 1);
                                lua_getglobal(mL, "description");
                                if (lua_isstring(mL, -1)) ext.description = lua_tostring(mL, -1);
                                lua_pop(mL, 1);
                                lua_getglobal(mL, "author");
                                if (lua_isstring(mL, -1)) ext.author = lua_tostring(mL, -1);
                                lua_pop(mL, 1);
                                lua_getglobal(mL, "version");
                                if (lua_isstring(mL, -1)) ext.version = lua_tostring(mL, -1);
                                lua_pop(mL, 1);
                            }
                            lua_close(mL);
                        }
                    }

                    extensions.push_back(ext);
                }
            }
        }

        log("Scanned " + std::to_string(extensions.size()) + " extensions.");
    }

    bool ExtensionManager::deleteExtension(const std::string& name) {
        auto root = std::filesystem::path(SettingsManager::get().rootDataPath);
        if (root.empty()) root = std::filesystem::current_path();
        auto extPath = root / "Extensions" / name;

        if (!std::filesystem::exists(extPath)) {
            log("Delete failed: extension '" + name + "' not found.", LogLevel::Error);
            return false;
        }

        try {
            std::filesystem::remove_all(extPath);
            log("Deleted extension: " + name, LogLevel::Success);
            scanExtensions();
            return true;
        } catch (const std::exception& e) {
            log("Failed to delete extension '" + name + "': " + e.what(), LogLevel::Error);
            return false;
        }
    }


    void ExtensionManager::installFactoryExtensions(bool force) {

        std::vector<std::filesystem::path> candidates;

        auto cwd = std::filesystem::current_path();
        auto exec = std::filesystem::path(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName().toStdString());
        auto execDir = exec.parent_path();

        candidates.push_back(cwd / "assets/Extensions");
        candidates.push_back(execDir / "assets/Extensions");
        candidates.push_back(execDir.parent_path() / "assets/Extensions");
        candidates.push_back(execDir.parent_path().parent_path() / "assets/Extensions");
        candidates.push_back(execDir.parent_path().parent_path().parent_path() / "assets/Extensions");

        std::filesystem::path sourceDir;
        for (const auto& p : candidates) {
            if (std::filesystem::exists(p) && std::filesystem::is_directory(p)) {
                sourceDir = p;
                break;
            }
        }

        if (sourceDir.empty()) {
            return;
        }


        auto root = std::filesystem::path(SettingsManager::get().rootDataPath);
        if (root.empty()) root = std::filesystem::current_path();
        auto targetDir = root / "Extensions";

        if (!std::filesystem::exists(targetDir)) {
            std::filesystem::create_directories(targetDir);
        }


        for (const auto& entry : std::filesystem::directory_iterator(sourceDir)) {
            if (entry.is_directory()) {
                auto extName = entry.path().filename();
                auto destPath = targetDir / extName;

                if (force && std::filesystem::exists(destPath)) {
                    try {
                        std::filesystem::remove_all(destPath);
                    } catch (const std::exception& e) {
                        log("Failed to remove existing extension " + extName.string() + ": " + e.what());
                    }
                }

                if (!std::filesystem::exists(destPath)) {
                    try {
                        std::filesystem::copy(entry.path(), destPath, std::filesystem::copy_options::recursive);
                        log("Installed factory extension: " + extName.string());
                    } catch (const std::exception& e) {
                        log("Failed to install factory extension " + extName.string() + ": " + e.what());
                    }
                }
            }
        }
    }

}
