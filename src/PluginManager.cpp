#include "orion/PluginManager.h"
#include "orion/VSTNode.h"
#include "orion/VST3Node.h"
#include "orion/GainNode.h"
#include "orion/EQ3Node.h"
#include "orion/DelayNode.h"
#include "orion/ReverbNode.h"
#include "orion/CompressorNode.h"
#include "orion/LimiterNode.h"
#include "orion/SynthesizerNode.h"
#include "orion/SamplerNode.h"
#include "orion/FluxShaperNode.h"
#include "orion/PrismStackNode.h"
#include "orion/Logger.h"
#include "orion/HostContext.h"
#include "orion/SettingsManager.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <fstream>
#include <set>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif


#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstcomponent.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

using namespace Steinberg;

typedef Steinberg::IPluginFactory* (PLUGIN_API *GetFactoryProc) ();

namespace Orion {


    static std::string getLastErrorString() {
#ifdef _WIN32
        DWORD error = GetLastError();
        if (error == 0) return "No error";
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);

        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        return message + " (Code: " + std::to_string(error) + ")";
#else
        const char* err = dlerror();
        return err ? std::string(err) : "Unknown error";
#endif
    }

    std::vector<PluginInfo> PluginManager::availablePlugins;
    std::mutex PluginManager::pluginMutex;
    std::atomic<bool> PluginManager::scanning(false);
    std::string PluginManager::currentScanPath;
    int PluginManager::scannedCount = 0;
    static std::map<VST2::AEffect*, void*> loadedVST2Modules;
    static std::mutex loadedModulesMutex;
    static constexpr int kPluginCacheSchemaVersion = 2;

    struct CacheRecord {
        std::string path;
        PluginFormat format = PluginFormat::VST3;
        uint64_t lastModified = 0;
        uint64_t fileSize = 0;
        bool scanSuccess = false;
        std::vector<PluginInfo> plugins;
    };

    static std::map<std::string, CacheRecord> pluginCache;
    static PluginCacheStats cacheStats;


    static thread_local VST2::VstTimeInfo timeInfo;

    static void ensureOrionPluginsLocked(std::vector<PluginInfo>& list, bool includeFluxShaper, bool includePrismStack)
    {
        auto addInternal = [&](const std::string& name, PluginType type, const std::string& category) {
            bool found = false;
            for (const auto& p : list) {
                if (p.name == name && p.format == PluginFormat::Internal) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                PluginInfo info;
                info.name = name;
                info.format = PluginFormat::Internal;
                info.type = type;
                info.category = category;
                info.vendor = "Orion";
                list.push_back(info);
            }
        };

        addInternal("Gain", PluginType::Effect, "Utility");
        addInternal("EQ3", PluginType::Effect, "EQ");
        addInternal("Compressor", PluginType::Effect, "Dynamics");
        addInternal("Limiter", PluginType::Effect, "Dynamics");
        addInternal("Delay", PluginType::Effect, "Time");
        addInternal("Reverb", PluginType::Effect, "Time");
        addInternal("Pulsar", PluginType::Instrument, "Instrument");
        addInternal("Comet", PluginType::Instrument, "Instrument");

        if (includeFluxShaper) addInternal("Flux Shaper", PluginType::Effect, "Modulation");
        if (includePrismStack) addInternal("Prism Stack", PluginType::Effect, "MIDI");
    }

    static VST2::VstIntPtr hostCallback(VST2::AEffect* effect, VST2::VstInt32 opcode, VST2::VstInt32 index, VST2::VstIntPtr value, void* ptr, float opt) {
        (void)effect; (void)index; (void)value; (void)ptr; (void)opt;
        switch (opcode) {
            case VST2::audioMasterVersion: return 2400;
            case VST2::audioMasterCurrentId: return 0xCAFEBABE;
            case VST2::audioMasterGetSampleRate:
                return (VST2::VstIntPtr)(gHostContext.sampleRate > 0 ? gHostContext.sampleRate : 44100);
            case VST2::audioMasterGetBlockSize: return 512;
            case VST2::audioMasterGetProductString:
                if (ptr) snprintf((char*)ptr, 64, "Orion");
                return 1;
            case VST2::audioMasterGetVendorString:
                if (ptr) snprintf((char*)ptr, 64, "WayV");
                return 1;
            case VST2::audioMasterGetTime: {
                memset(&timeInfo, 0, sizeof(timeInfo));
                timeInfo.sampleRate = gHostContext.sampleRate;
                timeInfo.samplePos = (double)gHostContext.projectTimeSamples;
                timeInfo.tempo = gHostContext.bpm;
                timeInfo.flags = VST2::kVstTempoValid | VST2::kVstPpqPosValid | VST2::kVstNanosValid;


                if (gHostContext.playing) {
                     timeInfo.flags |= VST2::kVstTransportPlaying;
                } else {
                     timeInfo.flags |= VST2::kVstTransportChanged;
                }


                double samplesPerBeat = 0.0;
                if (timeInfo.tempo > 0) {
                    samplesPerBeat = (60.0 / timeInfo.tempo) * timeInfo.sampleRate;
                    timeInfo.ppqPos = timeInfo.samplePos / samplesPerBeat;
                } else {
                    timeInfo.ppqPos = 0;
                }


                timeInfo.timeSigNumerator = (int)gHostContext.timeSigNumerator;
                timeInfo.timeSigDenominator = (int)gHostContext.timeSigDenominator;
                timeInfo.flags |= VST2::kVstTimeSigValid;


                if (timeInfo.tempo > 0 && timeInfo.timeSigDenominator > 0) {
                     double beatValue = 4.0 / (double)timeInfo.timeSigDenominator;
                     double ppqPerBar = (double)timeInfo.timeSigNumerator * beatValue;
                     if (ppqPerBar > 0) {
                         timeInfo.barStartPos = std::floor(timeInfo.ppqPos / ppqPerBar) * ppqPerBar;
                         timeInfo.flags |= VST2::kVstBarsValid;
                     }
                }


                if (gHostContext.loopActive) {
                    timeInfo.flags |= VST2::kVstCycleValid;
                    if (samplesPerBeat > 0) {
                        timeInfo.cycleStartPos = (double)gHostContext.loopStart / samplesPerBeat;
                        timeInfo.cycleEndPos = (double)gHostContext.loopEnd / samplesPerBeat;
                    }
                }

                return (VST2::VstIntPtr)&timeInfo;
            }
            case VST2::audioMasterCanDo: {
                 if (ptr) {
                     if (strcmp((char*)ptr, "sendVstEvents") == 0) return 1;
                     if (strcmp((char*)ptr, "sendVstMidiEvent") == 0) return 1;
                     if (strcmp((char*)ptr, "receiveVstEvents") == 0) return 1;
                     if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0) return 1;
                     if (strcmp((char*)ptr, "sizeWindow") == 0) return 1;
                 }
                 return 0;
            }
            default: return 0;
        }
    }

    VST2::AEffect* loadVST2Raw(const std::string& path) {
        Logger::info("Attempting to load VST2: " + path);
#ifdef _WIN32
        HMODULE handle = LoadLibraryA(path.c_str());
        if (!handle) {
            Logger::error("Failed to LoadLibrary " + path + ": " + getLastErrorString());
            return nullptr;
        }
        using MainProc = VST2::AEffect* (*)(VST2::audioMasterCallback);
        MainProc mainProc = (MainProc)GetProcAddress(handle, "VSTPluginMain");
        if (!mainProc) mainProc = (MainProc)GetProcAddress(handle, "main");
#else
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            Logger::error("Failed to dlopen " + path + ": " + getLastErrorString());
            return nullptr;
        }
        using MainProc = VST2::AEffect* (*)(VST2::audioMasterCallback);
        MainProc mainProc = (MainProc)dlsym(handle, "VSTPluginMain");
        if (!mainProc) mainProc = (MainProc)dlsym(handle, "main");
#endif
        if (!mainProc) {
            Logger::error("Entry point (VSTPluginMain/main) not found in " + path);
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
            return nullptr;
        }

        try {
            VST2::AEffect* effect = mainProc(hostCallback);
            if (effect && effect->magic == VST2::kEffectMagic) {
                effect->dispatcher(effect, VST2::effOpen, 0, 0, nullptr, 0.0f);

                std::lock_guard<std::mutex> lock(loadedModulesMutex);
                loadedVST2Modules[effect] = (void*)handle;
                Logger::info("Successfully loaded VST2: " + path);
                return effect;
            } else {
                Logger::error("Invalid VST2 magic number or null effect in " + path);
            }
        } catch (...) {
            Logger::error("Exception occurred during VST2 mainProc or effOpen in " + path);
        }

#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return nullptr;
    }

    std::shared_ptr<EffectNode> PluginManager::loadPlugin(const PluginInfo& info) {
        try {
            if (info.format == PluginFormat::Internal) {
                if (info.name == "Gain") return std::make_shared<GainNode>();
                if (info.name == "EQ3") return std::make_shared<EQ3Node>();
                if (info.name == "Delay") return std::make_shared<DelayNode>();
                if (info.name == "Reverb") return std::make_shared<ReverbNode>();
                if (info.name == "Compressor") return std::make_shared<CompressorNode>();
                if (info.name == "Limiter") return std::make_shared<LimiterNode>();
                if (info.name == "Pulsar" || info.name == "Synthesizer") return std::make_shared<SynthesizerNode>();
                if (info.name == "Comet" || info.name == "Sampler") return std::make_shared<SamplerNode>();
                if (info.name == "Flux Shaper") return std::make_shared<FluxShaperNode>();
                if (info.name == "Prism Stack") return std::make_shared<PrismStackNode>();
                return nullptr;
            } else if (info.format == PluginFormat::VST3) {
                auto node = std::make_shared<VST3Node>(info.path, info.subID);
                if (node->isValid()) {
                    return node;
                }
            } else {

                VST2::AEffect* effect = loadVST2Raw(info.path);
                if (effect) {
                return std::make_shared<VSTNode>(effect, info.name, info.path);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception loading plugin " << info.name << ": " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception loading plugin " << info.name << std::endl;
        }
        return nullptr;
    }

    std::string resolveVST3Path(const std::string& inputPath) {
        if (!fs::exists(inputPath)) return inputPath;

        if (fs::is_directory(inputPath)) {
            fs::path p(inputPath);





            std::vector<std::string> archs;
#ifdef _WIN32
            archs.push_back("x86_64-win");
#elif __linux__
            archs.push_back("x86_64-linux");
#elif __APPLE__
            archs.push_back("MacOS");
#endif

            for (const auto& arch : archs) {
                fs::path binDir = p / "Contents" / arch;
                if (fs::exists(binDir)) {

                    for (const auto& entry : fs::directory_iterator(binDir)) {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

#ifdef _WIN32
                        if (ext == ".vst3") return entry.path().string();
#else
                        if (ext == ".so" || ext.empty()) return entry.path().string();
#endif
                    }
                }
            }
        }
        return inputPath;
    }

    static uint64_t getPathLastModified(const fs::path& path) {
        try {
            return (uint64_t)fs::last_write_time(path).time_since_epoch().count();
        } catch (...) {
            return 0;
        }
    }

    static uint64_t getPathSize(const fs::path& path) {
        try {
            if (fs::is_regular_file(path)) return fs::file_size(path);
        } catch (...) {}
        return 0;
    }

    static bool metadataMatches(const CacheRecord& rec, uint64_t fileSize, uint64_t lastModified) {
        return rec.fileSize == fileSize && rec.lastModified == lastModified;
    }

    static std::string getPluginCachePathImpl() {
        auto dir = SettingsManager::getSettingsDirectory();
        if (dir.empty()) return "plugins.cache.json";
        fs::create_directories(dir);
        return (fs::path(dir) / "plugins.cache.json").string();
    }

    static void updateCacheRecord(const std::string& key,
                                  PluginFormat fmt,
                                  uint64_t fileSize,
                                  uint64_t lastModified,
                                  bool success,
                                  std::vector<PluginInfo> plugins)
    {
        CacheRecord rec;
        rec.path = key;
        rec.format = fmt;
        rec.fileSize = fileSize;
        rec.lastModified = lastModified;
        rec.scanSuccess = success;
        rec.plugins = std::move(plugins);
        pluginCache[key] = std::move(rec);
    }

    std::shared_ptr<EffectNode> PluginManager::loadPlugin(const std::string& rawPath) {
        std::string path = resolveVST3Path(rawPath);
        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        if (ext == ".vst3") {
            std::string cidStr;
#ifdef _WIN32
            HMODULE handle = LoadLibraryA(path.c_str());
            auto getFactory = (GetFactoryProc)(handle ? (void*)GetProcAddress(handle, "GetPluginFactory") : nullptr);
#else
            void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
            auto getFactory = (GetFactoryProc)(handle ? dlsym(handle, "GetPluginFactory") : nullptr);
#endif
            if (getFactory) {
                IPluginFactory* factory = getFactory();
                if (factory) {
                    int count = factory->countClasses();
                    for (int i = 0; i < count; ++i) {
                         PClassInfo info;
                         if (factory->getClassInfo(i, &info) == kResultOk) {
                             if (strcmp(info.category, "Audio Module Class") == 0) {
                                 char uidBuf[64];
                                 FUID(info.cid).toString(uidBuf);
                                 cidStr = uidBuf;
                                 break;
                             }
                         }
                    }
                }
            }
#ifdef _WIN32
            if (handle) FreeLibrary(handle);
#else
            if (handle) dlclose(handle);
#endif

            if (!cidStr.empty()) {
                auto node = std::make_shared<VST3Node>(path, cidStr);
                if (node->isValid()) return node;
            }
            return nullptr;
        } else {
            VST2::AEffect* effect = loadVST2Raw(path);
            if (effect) {
                return std::make_shared<VSTNode>(effect, fs::path(path).stem().string(), path);
            }
        }
        return nullptr;
    }

    void PluginManager::scanPlugins(bool force) {
        if (scanning.exchange(true)) return;
        Logger::info("Starting plugin scan (force: " + std::string(force ? "true" : "false") + ")...");
        Logger::info("OS: " + std::string(
#ifdef _WIN32
            "Windows"
#elif __linux__
            "Linux"
#elif __APPLE__
            "macOS"
#else
            "Unknown"
#endif
        ));
        std::thread(runScan, force).detach();
    }

    bool PluginManager::isScanning() {
        return scanning.load();
    }

    std::string PluginManager::getCurrentScanningFile() {
        std::lock_guard<std::mutex> lock(pluginMutex);
        return currentScanPath;
    }

    int PluginManager::getScannedCount() {
        std::lock_guard<std::mutex> lock(pluginMutex);
        return scannedCount;
    }

    std::string PluginManager::getCacheFilePath() {
        return getPluginCachePathImpl();
    }

    PluginCacheStats PluginManager::getCacheStats() {
        std::lock_guard<std::mutex> lock(pluginMutex);
        return cacheStats;
    }

    bool PluginManager::clearCache() {
        try {
            pluginCache.clear();
            {
                std::lock_guard<std::mutex> lock(pluginMutex);
                cacheStats = {};
                cacheStats.schemaVersion = kPluginCacheSchemaVersion;
            }
            const auto cacheFile = getPluginCachePathImpl();
            if (fs::exists(cacheFile)) fs::remove(cacheFile);
            return true;
        } catch (...) {
            return false;
        }
    }

    std::vector<PluginInfo> PluginManager::getAvailablePlugins() {
        std::lock_guard<std::mutex> lock(pluginMutex);
        const auto& settings = SettingsManager::get();
        ensureOrionPluginsLocked(availablePlugins, settings.fluxShaperEnabled, settings.prismStackEnabled);
        scannedCount = (int)availablePlugins.size();
        return availablePlugins;
    }



    NLOHMANN_JSON_SERIALIZE_ENUM(PluginFormat, {
        {PluginFormat::VST2, "VST2"},
        {PluginFormat::VST3, "VST3"},
        {PluginFormat::Internal, "Internal"}
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(PluginType, {
        {PluginType::Effect, "Effect"},
        {PluginType::Instrument, "Instrument"},
        {PluginType::Unknown, "Unknown"}
    })

    void to_json(json& j, const PluginInfo& p) {
        j = json{
            {"name", p.name},
            {"vendor", p.vendor},
            {"category", p.category},
            {"path", p.path},
            {"subID", p.subID},
            {"format", p.format},
            {"type", p.type},
            {"lastModified", p.lastModified},
            {"fileSize", p.fileSize}
        };
    }

    void from_json(const json& j, PluginInfo& p) {
        j.at("name").get_to(p.name);
        j.at("vendor").get_to(p.vendor);
        j.at("category").get_to(p.category);
        j.at("path").get_to(p.path);
        j.at("subID").get_to(p.subID);
        j.at("format").get_to(p.format);
        j.at("type").get_to(p.type);
        if(j.contains("lastModified")) j.at("lastModified").get_to(p.lastModified);
        if(j.contains("fileSize")) j.at("fileSize").get_to(p.fileSize);
    }

    void to_json(json& j, const CacheRecord& c) {
        j = json{
            {"path", c.path},
            {"format", c.format},
            {"lastModified", c.lastModified},
            {"fileSize", c.fileSize},
            {"scanSuccess", c.scanSuccess},
            {"plugins", c.plugins}
        };
    }

    void from_json(const json& j, CacheRecord& c) {
        j.at("path").get_to(c.path);
        j.at("format").get_to(c.format);
        if (j.contains("lastModified")) j.at("lastModified").get_to(c.lastModified);
        if (j.contains("fileSize")) j.at("fileSize").get_to(c.fileSize);
        if (j.contains("scanSuccess")) j.at("scanSuccess").get_to(c.scanSuccess);
        if (j.contains("plugins")) j.at("plugins").get_to(c.plugins);
    }

    void PluginManager::loadCache() {
        pluginCache.clear();
        {
            std::lock_guard<std::mutex> lock(pluginMutex);
            cacheStats = {};
            cacheStats.schemaVersion = kPluginCacheSchemaVersion;
        }

        const auto cacheFile = getPluginCachePathImpl();
        if (!fs::exists(cacheFile)) return;
        try {
            std::ifstream f(cacheFile);
            json j;
            f >> j;

            int schema = j.value("schemaVersion", 1);
            {
                std::lock_guard<std::mutex> lock(pluginMutex);
                cacheStats.schemaVersion = schema;
            }
            if (schema != kPluginCacheSchemaVersion) {
                Logger::warn("Plugin cache schema mismatch; rebuilding cache.");
                return;
            }

            auto records = j.value("records", std::vector<CacheRecord>{});
            for (const auto& rec : records) {
                pluginCache[rec.path] = rec;
            }

            {
                std::lock_guard<std::mutex> lock(pluginMutex);
                availablePlugins.clear();
                scannedCount = 0;
                for (const auto& kv : pluginCache) {
                    if (!kv.second.scanSuccess) continue;
                    availablePlugins.insert(availablePlugins.end(), kv.second.plugins.begin(), kv.second.plugins.end());
                    scannedCount += (int)kv.second.plugins.size();
                }
                cacheStats.recordCount = (int)pluginCache.size();
                cacheStats.pluginCount = scannedCount;
            }
        } catch(...) {
            Logger::error("Failed to load plugin cache.");
        }
    }

    void PluginManager::saveCache() {
        try {
            std::vector<CacheRecord> records;
            records.reserve(pluginCache.size());
            for (const auto& kv : pluginCache) {
                records.push_back(kv.second);
            }

            json j;
            j["schemaVersion"] = kPluginCacheSchemaVersion;
            j["records"] = records;

            const auto cacheFile = getPluginCachePathImpl();
            std::ofstream f(cacheFile);
            f << j.dump(4);
        } catch(...) {
            Logger::error("Failed to save plugin cache.");
        }
    }

    void PluginManager::runScan(bool force) {
        const auto& settings = SettingsManager::get();
        const bool useCache = settings.pluginCacheEnabled && !force;
        if (useCache) loadCache();
        else pluginCache.clear();

        {
            std::lock_guard<std::mutex> lock(pluginMutex);
            availablePlugins.clear();
            scannedCount = 0;
            currentScanPath = "Initializing...";
        }






        std::vector<std::string> vst2Paths;
        std::vector<std::string> vst3Paths;

#ifdef _WIN32
        vst2Paths = {
            "C:\\Program Files\\VSTPlugins",
            "C:\\Program Files\\Steinberg\\VSTPlugins",
            "C:\\Program Files (x86)\\VSTPlugins",
            "C:\\Program Files (x86)\\Steinberg\\VSTPlugins"
        };
        vst3Paths = {
            "C:\\Program Files\\Common Files\\VST3"
        };
#else
        vst2Paths = {
            "/usr/lib/vst",
            "/usr/local/lib/vst",
            std::string(getenv("HOME") ? getenv("HOME") : ".") + "/.vst"
        };
        vst3Paths = {
            "/usr/lib/vst3",
            "/usr/local/lib/vst3",
            std::string(getenv("HOME") ? getenv("HOME") : ".") + "/.vst3"
        };
#endif
        if (auto p = getenv("VST_PATH")) vst2Paths.push_back(p);
        if (auto p = getenv("VST3_PATH")) vst3Paths.push_back(p);


        vst2Paths.insert(vst2Paths.end(), settings.vst2Paths.begin(), settings.vst2Paths.end());
        vst3Paths.insert(vst3Paths.end(), settings.vst3Paths.begin(), settings.vst3Paths.end());
        {
            std::lock_guard<std::mutex> lock(pluginMutex);
            cacheStats.hitCount = 0;
            cacheStats.missCount = 0;
        }

        scanVST2(vst2Paths);
        scanVST3(vst3Paths);

        {
            std::lock_guard<std::mutex> lock(pluginMutex);
            if (!settings.fluxShaperEnabled) {
                auto it = std::remove_if(availablePlugins.begin(), availablePlugins.end(), [](const PluginInfo& p){
                    return p.format == PluginFormat::Internal && p.name == "Flux Shaper";
                });
                if (it != availablePlugins.end()) availablePlugins.erase(it, availablePlugins.end());
            }

            if (!settings.prismStackEnabled) {
                auto it = std::remove_if(availablePlugins.begin(), availablePlugins.end(), [](const PluginInfo& p){
                    return p.format == PluginFormat::Internal && p.name == "Prism Stack";
                });
                if (it != availablePlugins.end()) availablePlugins.erase(it, availablePlugins.end());
            }

            ensureOrionPluginsLocked(availablePlugins, settings.fluxShaperEnabled, settings.prismStackEnabled);

            // Deduplicate: If same plugin exists in VST2 and VST3, keep VST3
            std::map<std::string, size_t> vst3Plugins;
            for (size_t i = 0; i < availablePlugins.size(); ++i) {
                if (availablePlugins[i].format == PluginFormat::VST3) {
                    vst3Plugins[availablePlugins[i].name] = i;
                }
            }

            auto it = std::remove_if(availablePlugins.begin(), availablePlugins.end(), [&](const PluginInfo& p) {
                if (p.format == PluginFormat::VST2) {
                    return vst3Plugins.count(p.name) > 0;
                }
                return false;
            });
            if (it != availablePlugins.end()) {
                availablePlugins.erase(it, availablePlugins.end());
            }

            scannedCount = (int)availablePlugins.size();
            cacheStats.recordCount = (int)pluginCache.size();
            cacheStats.pluginCount = scannedCount;
            cacheStats.schemaVersion = kPluginCacheSchemaVersion;
        }

        if (settings.pluginCacheEnabled) saveCache();

        scanning.store(false);
    }

    void PluginManager::scanVST2(const std::vector<std::string>& paths) {
        for (const auto& path : paths) {
            if (!fs::exists(path)) {
                 Logger::warn("VST2 Path does not exist: " + path);
                 continue;
            }
            Logger::info("Scanning VST2 Path: " + path);
            try {
                for (const auto& entry : fs::recursive_directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

                        bool isDll = false;
#ifdef _WIN32
                        if (ext == ".dll") isDll = true;
#else
                        if (ext == ".so") isDll = true;
#endif
                        if (isDll) {
                            std::string p = entry.path().string();

                            {
                                std::lock_guard<std::mutex> lock(pluginMutex);
                                currentScanPath = p;
                            }


                            bool needsScan = true;
                            uint64_t fSize = getPathSize(entry.path());
                            uint64_t fTime = getPathLastModified(entry.path());

                            auto cacheIt = pluginCache.find(p);
                            if (cacheIt != pluginCache.end() && metadataMatches(cacheIt->second, fSize, fTime)) {
                                if (cacheIt->second.scanSuccess) {
                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    availablePlugins.insert(availablePlugins.end(),
                                                            cacheIt->second.plugins.begin(),
                                                            cacheIt->second.plugins.end());
                                    scannedCount += (int)cacheIt->second.plugins.size();
                                }
                                {
                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    cacheStats.hitCount++;
                                }
                                needsScan = false;
                            }

                            if (needsScan) {
                                bool valid = false;
                                std::string errorReason;

#ifdef _WIN32

                                HMODULE handle = LoadLibraryExA(p.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
                                if (handle) {
                                    if (GetProcAddress(handle, "VSTPluginMain") || GetProcAddress(handle, "main")) {
                                        valid = true;
                                    } else {
                                        errorReason = "No VSTPluginMain/main export";
                                    }
                                    FreeLibrary(handle);
                                } else {
                                    errorReason = "LoadLibraryEx failed: " + getLastErrorString();
                                }
#else
                                valid = true;
#endif
                                if (valid) {
                                    PluginInfo info;
                                    info.name = entry.path().stem().string();
                                    info.path = p;
                                    info.format = PluginFormat::VST2;
                                    info.type = PluginType::Unknown;
                                    info.category = "VST2";
                                    info.vendor = "Unknown";
                                    info.fileSize = fSize;
                                    info.lastModified = fTime;

                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    availablePlugins.push_back(info);
                                    scannedCount++;
                                    Logger::info("Found VST2: " + info.name);
                                    updateCacheRecord(p, PluginFormat::VST2, fSize, fTime, true, {info});
                                } else {
                                    Logger::warn("Skipping VST2 candidate " + p + ": " + errorReason);
                                    updateCacheRecord(p, PluginFormat::VST2, fSize, fTime, false, {});
                                }
                                {
                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    cacheStats.missCount++;
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                 Logger::error("Exception scanning directory " + path + ": " + e.what());
            } catch (...) {
                 Logger::error("Unknown exception scanning directory " + path);
            }
        }
    }

    void PluginManager::scanVST3(const std::vector<std::string>& paths) {
        for (const auto& path : paths) {
            if (!fs::exists(path)) {
                Logger::warn("VST3 Path does not exist: " + path);
                continue;
            }
            Logger::info("Scanning VST3 Path: " + path);
            try {
                std::set<std::string> candidates;
                for (const auto& entry : fs::recursive_directory_iterator(path)) {
                    const auto candidatePath = entry.path();
                    std::string ext = candidatePath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
                    if (ext != ".vst3") continue;
                    if (entry.is_regular_file() || entry.is_directory()) {
                        candidates.insert(candidatePath.string());
                    }
                }

                for (const auto& p : candidates) {
                    fs::path pluginPath(p);

                    {
                        std::lock_guard<std::mutex> lock(pluginMutex);
                        currentScanPath = p;
                    }


                    bool needsScan = true;
                    uint64_t fSize = getPathSize(pluginPath);
                    uint64_t fTime = getPathLastModified(pluginPath);

                    auto cacheIt = pluginCache.find(p);
                    if (cacheIt != pluginCache.end() && metadataMatches(cacheIt->second, fSize, fTime)) {
                        if (cacheIt->second.scanSuccess) {
                            std::lock_guard<std::mutex> lock(pluginMutex);
                            availablePlugins.insert(availablePlugins.end(),
                                                    cacheIt->second.plugins.begin(),
                                                    cacheIt->second.plugins.end());
                            scannedCount += (int)cacheIt->second.plugins.size();
                        }
                        {
                            std::lock_guard<std::mutex> lock(pluginMutex);
                            cacheStats.hitCount++;
                        }
                        needsScan = false;
                    }

                    if (needsScan) {
                        try {
                            std::vector<PluginInfo> discovered;
                            std::string resolvedPath = resolveVST3Path(p);
#ifdef _WIN32
                            HMODULE handle = LoadLibraryA(resolvedPath.c_str());
                            if (!handle) {
                                Logger::error("Failed to load VST3 library " + resolvedPath + ": " + getLastErrorString());
                                updateCacheRecord(p, PluginFormat::VST3, fSize, fTime, false, {});
                                {
                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    cacheStats.missCount++;
                                }
                                continue;
                            }
                            auto getFactory = (GetFactoryProc)GetProcAddress(handle, "GetPluginFactory");
#else
                            void* handle = dlopen(resolvedPath.c_str(), RTLD_NOW | RTLD_LOCAL);
                            if (!handle) {
                                Logger::error("Failed to dlopen VST3 " + resolvedPath + ": " + getLastErrorString());
                                updateCacheRecord(p, PluginFormat::VST3, fSize, fTime, false, {});
                                {
                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    cacheStats.missCount++;
                                }
                                continue;
                            }
                            auto getFactory = (GetFactoryProc)dlsym(handle, "GetPluginFactory");
#endif
                            if (getFactory) {
                                IPluginFactory* factory = getFactory();
                                if (factory) {
                                    IPluginFactory2* factory2 = nullptr;
                                    factory->queryInterface(IPluginFactory2::iid, (void**)&factory2);

                                    PFactoryInfo factoryInfo;
                                    std::string vendor = "Unknown";
                                    if (factory->getFactoryInfo(&factoryInfo) == kResultOk) {
                                        vendor = factoryInfo.vendor;
                                    }

                                    int numClasses = factory->countClasses();
                                    for (int i = 0; i < numClasses; ++i) {
                                        PClassInfo info;
                                        PClassInfo2 info2;
                                        bool hasInfo2 = false;

                                        if (factory2 && factory2->getClassInfo2(i, &info2) == kResultOk) {
                                            hasInfo2 = true;
                                            strcpy(info.category, info2.category);
                                            strcpy(info.name, info2.name);
                                            memcpy(info.cid, info2.cid, sizeof(TUID));
                                        } else {
                                            if (factory->getClassInfo(i, &info) != kResultOk) continue;
                                        }

                                        if (strcmp(info.category, "Audio Module Class") == 0) {
                                            PluginInfo pi;
                                            pi.name = info.name;
                                            pi.path = resolvedPath;
                                            pi.vendor = vendor;

                                            char uidBuf[64];
                                            FUID(info.cid).toString(uidBuf);
                                            pi.subID = std::string(uidBuf);
                                            pi.format = PluginFormat::VST3;
                                            pi.fileSize = fSize;
                                            pi.lastModified = fTime;

                                            std::string subCats;
                                            if (hasInfo2) subCats = info2.subCategories;

                                            if (!subCats.empty()) {
                                                 pi.category = subCats;
                                            } else {
                                                 pi.category = "Effect";
                                            }

                                            if (subCats.find("Instrument") != std::string::npos) {
                                                pi.type = PluginType::Instrument;
                                            } else {
                                                pi.type = PluginType::Effect;
                                            }
                                            discovered.push_back(pi);
                                        }
                                    }
                                    if (factory2) factory2->release();
                                } else {
                                    Logger::error("VST3 GetPluginFactory returned null: " + p);
                                }
                            } else {
                                Logger::error("VST3 GetPluginFactory symbol not found in " + p);
                            }
#ifdef _WIN32
                            FreeLibrary(handle);
#else
                            if (handle) dlclose(handle);
#endif
                            if (!discovered.empty()) {
                                {
                                    std::lock_guard<std::mutex> lock(pluginMutex);
                                    availablePlugins.insert(availablePlugins.end(), discovered.begin(), discovered.end());
                                    scannedCount += (int)discovered.size();
                                }
                                updateCacheRecord(p, PluginFormat::VST3, fSize, fTime, true, discovered);
                                for (const auto& d : discovered) Logger::info("Found VST3: " + d.name);
                            } else {
                                updateCacheRecord(p, PluginFormat::VST3, fSize, fTime, false, {});
                            }
                            {
                                std::lock_guard<std::mutex> lock(pluginMutex);
                                cacheStats.missCount++;
                            }
                        } catch (const std::exception& e) {
                             Logger::error("Exception scanning VST3 " + p + ": " + e.what());
                             updateCacheRecord(p, PluginFormat::VST3, fSize, fTime, false, {});
                             {
                                 std::lock_guard<std::mutex> lock(pluginMutex);
                                 cacheStats.missCount++;
                             }
                        } catch (...) {
                             Logger::error("Unknown exception scanning VST3 " + p);
                             updateCacheRecord(p, PluginFormat::VST3, fSize, fTime, false, {});
                             {
                                 std::lock_guard<std::mutex> lock(pluginMutex);
                                 cacheStats.missCount++;
                             }
                        }
                    }
                }
            } catch (const std::exception& e) {
                Logger::error("Exception scanning VST3 dir " + path + ": " + e.what());
            } catch (...) {
                Logger::error("Unknown exception scanning VST3 dir " + path);
            }
        }
    }

    void PluginManager::unloadPlugin(VST2::AEffect* effect) {
        if (!effect) return;
        std::lock_guard<std::mutex> lock(loadedModulesMutex);
        auto it = loadedVST2Modules.find(effect);
        if (it != loadedVST2Modules.end()) {
            void* handle = it->second;
#ifdef _WIN32
            FreeLibrary((HMODULE)handle);
#else
            dlclose(handle);
#endif
            loadedVST2Modules.erase(it);
        }
    }

}
