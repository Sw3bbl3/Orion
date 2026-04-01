#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>
#include "orion/SettingsManager.h"
#include "orion/Logger.h"

int main() {
    std::cout << "Running Settings Migration Test..." << std::endl;

    // 1. Cleanup previous runs
    if (std::filesystem::exists("settings.json")) std::filesystem::remove("settings.json");
    if (std::filesystem::exists("settings.orionconf")) std::filesystem::remove("settings.orionconf");
    if (std::filesystem::exists("orion_location.txt")) std::filesystem::remove("orion_location.txt");
    if (std::filesystem::exists("test_data")) std::filesystem::remove_all("test_data");

    // 2. Test Migration (Legacy .json -> .orionconf)
    std::cout << "Testing Migration..." << std::endl;
    {
        std::ofstream f("settings.json");
        f << "{\"sampleRate\": 96000}";
        f.close();
    }

    // Load first to trigger migration
    Orion::SettingsManager::load();

    if (std::filesystem::exists("settings.json")) {
         std::cerr << "Error: Legacy settings.json not migrated." << std::endl;
         return 1;
    }
    if (!std::filesystem::exists("settings.orionconf")) {
         std::cerr << "Error: settings.orionconf not created after migration." << std::endl;
         return 1;
    }
    if (Orion::SettingsManager::get().sampleRate != 96000) {
        std::cerr << "Error: Migration failed to preserve data. Expected 96000, got " << Orion::SettingsManager::get().sampleRate << std::endl;
        return 1;
    }
    std::cout << "Migration Successful." << std::endl;

    // 3. Create/Save settings
    Orion::AppSettings initial = Orion::SettingsManager::get();
    initial.sampleRate = 48000;
    initial.setupComplete = false;
    Orion::SettingsManager::set(initial); // This should save to "settings.orionconf"

    // Verify file exists
    if (!std::filesystem::exists("settings.orionconf")) {
        std::cerr << "Error: settings.orionconf not created." << std::endl;
        return 1;
    }

    // 4. Move to new location
    std::string newPath = "test_data/Orion";
    std::filesystem::create_directories(newPath);

    std::cout << "Setting instance root to: " + newPath << std::endl;
    Orion::SettingsManager::setInstanceRoot(newPath);

    // 5. Verify redirection file
    if (!std::filesystem::exists("orion_location.txt")) {
        std::cerr << "Error: orion_location.txt not created." << std::endl;
        return 1;
    }
    {
        std::ifstream f("orion_location.txt");
        std::string s;
        std::getline(f, s);
        std::cout << "Redirection file content: " << s << std::endl;
    }

    // 6. Verify file moved
    if (std::filesystem::exists("settings.orionconf")) {
         std::cout << "WARNING: Old settings.orionconf still exists." << std::endl;
    } else {
         std::cout << "Old settings.orionconf removed." << std::endl;
    }

    if (!std::filesystem::exists("test_data/Orion/settings.orionconf")) {
        std::cerr << "Error: New settings.orionconf not found." << std::endl;
        return 1;
    }
    std::cout << "New settings.orionconf exists." << std::endl;

    // 6. Reload and Verify
    // Create a new SettingsManager cycle simulation
    // Since everything is static, we can just call load()
    Orion::SettingsManager::load();

    if (Orion::SettingsManager::get().sampleRate != 48000) {
        std::cerr << "Error: Sample rate mismatch. Expected 48000, got " << Orion::SettingsManager::get().sampleRate << std::endl;
        return 1;
    }

    // Check root path in memory
    if (Orion::SettingsManager::get().rootDataPath != newPath) {
         std::cerr << "Error: rootDataPath mismatch. Expected " << newPath << ", got " << Orion::SettingsManager::get().rootDataPath << std::endl;
         return 1;
    }

    std::cout << "Test Passed!" << std::endl;

    // Cleanup
    // std::filesystem::remove("settings.json");
    // std::filesystem::remove("orion_location.txt");
    // std::filesystem::remove_all("test_data");

    return 0;
}
