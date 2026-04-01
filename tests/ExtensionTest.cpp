#include "ExtensionManager.h"
#include "ProjectManager.h"
#include "orion/AudioEngine.h"
#include "orion/SettingsManager.h"
#include "orion/InstrumentTrack.h"
#include "orion/MidiClip.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>

using namespace Orion;

// Helper to run script from file
void runScript(ExtensionManager& em, const std::string& path) {
    if (!em.runScript(path)) {
        std::cerr << "Script failed: " << path << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "Starting ExtensionTest..." << std::endl;

    // Setup Environment
    // Use current path as root for Extensions folder discovery
    SettingsManager::get().rootDataPath = std::filesystem::current_path().string();

    AudioEngine engine;
    ProjectManager pm;
    ExtensionManager& em = ExtensionManager::getInstance();

    em.setContext(&engine, &pm);
    em.initialize(); // This will scan extensions but we will run scripts manually

    // 1. Test Lo-Fi Chords
    std::cout << "Testing Lo-Fi Chords..." << std::endl;
    runScript(em, "assets/Extensions/LoFiChords/main.lua");

    // Verify Track Created
    if (engine.getTracks().size() != 1) {
        std::cerr << "Lo-Fi Chords failed: Track count is " << engine.getTracks().size() << ", expected 1." << std::endl;
        return 1;
    }

    auto track1 = engine.getTracks()[0];
    if (track1->getName() != "Lo-Fi Keys") {
        std::cerr << "Lo-Fi Chords failed: Track name is " << track1->getName() << std::endl;
        return 1;
    }

    auto* instTrack1 = dynamic_cast<InstrumentTrack*>(track1.get());
    if (!instTrack1) {
        std::cerr << "Lo-Fi Chords failed: Track is not InstrumentTrack." << std::endl;
        return 1;
    }

    if (instTrack1->getMidiClips()->size() != 1) {
        std::cerr << "Lo-Fi Chords failed: Clip count is " << instTrack1->getMidiClips()->size() << std::endl;
        return 1;
    }

    auto clip1 = (*instTrack1->getMidiClips())[0];
    // Check notes. 3 chords * (5 + 4 + 5) = 14 notes.
    if (clip1->getNotes().size() != 14) {
        std::cerr << "Lo-Fi Chords failed: Note count is " << clip1->getNotes().size() << ", expected 14." << std::endl;
        return 1;
    }

    std::cout << "Lo-Fi Chords Passed." << std::endl;

    // 2. Test Techno Rumble
    std::cout << "Testing Techno Rumble..." << std::endl;
    runScript(em, "assets/Extensions/TechnoRumble/main.lua");

    if (engine.getTracks().size() != 3) {
        std::cerr << "Techno Rumble failed: Track count is " << engine.getTracks().size() << ", expected 3." << std::endl;
        return 1;
    }

    auto track2 = engine.getTracks()[1];
    if (track2->getName() != "Techno Kick") {
        std::cerr << "Techno Rumble failed: Track 2 name is " << track2->getName() << std::endl;
        return 1;
    }

    auto* instTrack2 = dynamic_cast<InstrumentTrack*>(track2.get());
    auto kickClip = (*instTrack2->getMidiClips())[0];
    if (kickClip->getNotes().size() != 16) {
        std::cerr << "Techno Rumble failed: Kick notes " << kickClip->getNotes().size() << ", expected 16." << std::endl;
        return 1;
    }

    auto track3 = engine.getTracks()[2];
    if (track3->getName() != "Rumble Bass") {
        std::cerr << "Techno Rumble failed: Track 3 name is " << track3->getName() << std::endl;
        return 1;
    }

    auto* instTrack3 = dynamic_cast<InstrumentTrack*>(track3.get());
    auto rumbleClip = (*instTrack3->getMidiClips())[0];
    // 16 beats * 3 notes = 48 notes
    if (rumbleClip->getNotes().size() != 48) {
        std::cerr << "Techno Rumble failed: Rumble notes " << rumbleClip->getNotes().size() << ", expected 48." << std::endl;
        return 1;
    }

    std::cout << "Techno Rumble Passed." << std::endl;

    // 3. Test Project Cleaner
    std::cout << "Testing Project Cleaner..." << std::endl;
    // Add empty track
    em.runCode("orion.track.addAudioTrack()");
    if (engine.getTracks().size() != 4) {
        std::cerr << "Project Cleaner Setup failed: Track count " << engine.getTracks().size() << std::endl;
        return 1;
    }

    // Modify volume of track 1 to check reset
    engine.getTracks()[0]->setVolume(0.5f);

    runScript(em, "assets/Extensions/ProjectCleaner/main.lua");

    if (engine.getTracks().size() != 3) {
        std::cerr << "Project Cleaner failed: Track count " << engine.getTracks().size() << ", expected 3." << std::endl;
        return 1;
    }

    // Check if track 1 volume is reset to 1.0
    if (std::abs(engine.getTracks()[0]->getVolume() - 1.0f) > 0.001f) {
        std::cerr << "Project Cleaner failed: Volume not reset. Got " << engine.getTracks()[0]->getVolume() << std::endl;
        return 1;
    }

    std::cout << "Project Cleaner Passed." << std::endl;

    // 4. Test Marker Helper
    std::cout << "Testing Marker Helper..." << std::endl;
    engine.setBpm(120.0f);
    runScript(em, "assets/Extensions/MarkerHelper/main.lua");
    
    // BPM 120, 8 bars = 16 seconds. Should have added one marker at 0 and one at 16s.
    // Wait, the script adds at 0, 8, 16, 24, 32 bars.
    if (engine.getRegionMarkers().size() < 2) {
        std::cerr << "Marker Helper failed: Marker count " << engine.getRegionMarkers().size() << std::endl;
        return 1;
    }
    std::cout << "Marker Helper Passed." << std::endl;

    // 5. Test Mix Reset All
    std::cout << "Testing Mix Reset All..." << std::endl;
    engine.getTracks()[0]->setVolume(0.2f);
    engine.getTracks()[0]->setPan(-0.5f);
    runScript(em, "assets/Extensions/MixResetAll/main.lua");
    
    if (std::abs(engine.getTracks()[0]->getVolume() - 1.0f) > 0.001f || std::abs(engine.getTracks()[0]->getPan() - 0.0f) > 0.001f) {
        std::cerr << "Mix Reset All failed: Values not reset correctly." << std::endl;
        return 1;
    }
    std::cout << "Mix Reset All Passed." << std::endl;

    em.shutdown();
    std::cout << "All Extension Tests Passed." << std::endl;
    return 0;
}
