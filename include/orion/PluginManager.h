#pragma once

#include "EffectNode.h"
#include "orion/VST2.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Orion {

    enum class PluginFormat {
        VST2,
        VST3,
        Internal
    };

    enum class PluginType {
        Effect,
        Instrument,
        Unknown
    };

    struct PluginInfo {
        std::string name;
        std::string vendor;
        std::string category;
        std::string path;
        std::string subID;
        PluginFormat format;
        PluginType type;
        uint64_t lastModified = 0;
        uint64_t fileSize = 0;
    };

    struct PluginCacheStats {
        int schemaVersion = 0;
        int recordCount = 0;
        int pluginCount = 0;
        int hitCount = 0;
        int missCount = 0;
    };

    class PluginManager {
    public:

        static std::shared_ptr<EffectNode> loadPlugin(const PluginInfo& info);

        static std::shared_ptr<EffectNode> loadPlugin(const std::string& path);


        static void scanPlugins(bool force = false);
        static std::vector<PluginInfo> getAvailablePlugins();
        static bool isScanning();
        static std::string getCurrentScanningFile();
        static int getScannedCount();
        static std::string getCacheFilePath();
        static PluginCacheStats getCacheStats();
        static bool clearCache();

        static void unloadPlugin(VST2::AEffect* effect);
        static void loadCache();

    private:
        static std::vector<PluginInfo> availablePlugins;
        static std::mutex pluginMutex;
        static std::atomic<bool> scanning;
        static std::string currentScanPath;
        static int scannedCount;

        static void scanVST2(const std::vector<std::string>& paths);
        static void scanVST3(const std::vector<std::string>& paths);

        static void runScan(bool force);
        static void saveCache();
    };

}
