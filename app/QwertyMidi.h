#pragma once
#include <JuceHeader.h>
#include <map>
#include <vector>
#include <functional>

namespace Orion {

    class QwertyMidi {
    public:
        QwertyMidi();


        void process(std::function<void(const juce::MidiMessage&)> callback);

        void setOctave(int oct) { octave = oct; }
        int getOctave() const { return octave; }

        void setVelocity(float vel) { velocity = vel; }

    private:
        struct KeyMap {
            int keyCode;
            int noteOffset;
        };

        std::vector<KeyMap> keyMappings;
        std::vector<int> activeKeys;

        int octave = 0;
        float velocity = 0.8f;

        void addMapping(int key, int note);
    };

}
