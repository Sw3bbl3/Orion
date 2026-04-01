#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "orion/AudioClip.h"
#include "orion/GlitchProcessor.h"
#include "orion/FluxShaperNode.h"
#include "orion/PrismStackNode.h"
#include "orion/AudioBlock.h"
#include "orion/HostContext.h"

namespace Orion {
    thread_local HostContext gHostContext;
}

int main() {
    std::cout << "Verifying Glitch Brush Logic..." << std::endl;

    auto clip = std::make_shared<Orion::AudioClip>();
    clip->setLengthFrames(44100 * 4); // 4 seconds
    clip->setName("Test Clip");

    Orion::GlitchSettings settings;
    settings.type = Orion::GlitchType::Reverse;

    Orion::GlitchProcessor::applyGlitch(clip, 0, 0, 120.0, settings);

    if (clip->getName() == "Rev" && clip->isReverse()) {
        std::cout << "[PASS] Glitch Brush Reverse applied correctly." << std::endl;
    } else {
        std::cout << "[FAIL] Glitch Brush Reverse failed." << std::endl;
        return 1;
    }

    settings.type = Orion::GlitchType::TapeStop;
    Orion::GlitchProcessor::applyGlitch(clip, 0, 0, 120.0, settings);
    if (clip->getName() == "Stop" && clip->getFadeOut() > 0) {
        std::cout << "[PASS] Glitch Brush TapeStop applied correctly." << std::endl;
    } else {
        std::cout << "[FAIL] Glitch Brush TapeStop failed." << std::endl;
        return 1;
    }


    std::cout << "Verifying Flux Shaper Logic..." << std::endl;
    auto flux = std::make_shared<Orion::FluxShaperNode>();
    Orion::AudioBlock block(2, 128);
    // Fill block with 1.0
    for(size_t c=0; c<2; ++c) {
        for(size_t i=0; i<128; ++i) block.getChannelPointer(c)[i] = 1.0f;
    }

    Orion::gHostContext.bpm = 120.0;
    Orion::gHostContext.sampleRate = 44100.0;
    Orion::gHostContext.projectTimeSamples = 0;

    flux->process(block, 0);

    // Check if gain was applied (should be < 1.0 at start of ramp if point 0 is 0.0)
    // Default FluxShaper points start at 0.0 gain.
    if (block.getChannelPointer(0)[0] < 0.1f) {
        std::cout << "[PASS] Flux Shaper applied ducking." << std::endl;
    } else {
        std::cout << "[FAIL] Flux Shaper gain seems wrong: " << block.getChannelPointer(0)[0] << std::endl;
        return 1;
    }

    std::cout << "Verifying Prism Stack Logic..." << std::endl;
    auto prism = std::make_shared<Orion::PrismStackNode>();
    prism->setChordType(Orion::PrismStackNode::Major);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0); // C4

    prism->processMidi(midi, 128);

    int noteCount = 0;
    for(const auto metadata : midi) {
        if(metadata.getMessage().isNoteOn()) noteCount++;
    }

    // Major chord = Root + 3 offsets (Root, +4, +7, +12) -> 4 notes
    // Wait, PrismStackNode logic:
    // offsets = {4, 7, 12}
    // + Root itself is added first.
    // So 1 + 3 = 4 notes.

    if (noteCount == 4) {
        std::cout << "[PASS] Prism Stack generated Major chord." << std::endl;
    } else {
        std::cout << "[FAIL] Prism Stack generated " << noteCount << " notes, expected 4." << std::endl;
        return 1;
    }

    std::cout << "All verification tests passed!" << std::endl;
    return 0;
}
