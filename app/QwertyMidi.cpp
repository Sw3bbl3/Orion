#include "QwertyMidi.h"
#include <algorithm>

namespace Orion {

    QwertyMidi::QwertyMidi() {

        addMapping('Z', 0);
        addMapping('S', 1);
        addMapping('X', 2);
        addMapping('D', 3);
        addMapping('C', 4);
        addMapping('V', 5);
        addMapping('G', 6);
        addMapping('B', 7);
        addMapping('H', 8);
        addMapping('N', 9);
        addMapping('J', 10);
        addMapping('M', 11);


        addMapping(',', 12);
        addMapping('L', 13);
        addMapping('.', 14);
        addMapping(';', 15);
        addMapping('/', 16);


        addMapping('Q', 12);
        addMapping('2', 13);
        addMapping('W', 14);
        addMapping('3', 15);
        addMapping('E', 16);
        addMapping('R', 17);
        addMapping('5', 18);
        addMapping('T', 19);
        addMapping('6', 20);
        addMapping('Y', 21);
        addMapping('7', 22);
        addMapping('U', 23);


        addMapping('I', 24);
        addMapping('9', 25);
        addMapping('O', 26);
        addMapping('0', 27);
        addMapping('P', 28);
    }

    void QwertyMidi::addMapping(int key, int note) {
        keyMappings.push_back({key, note});
    }

    void QwertyMidi::process(std::function<void(const juce::MidiMessage&)> callback) {
        int baseNote = 60 + (octave * 12);

        for (const auto& mapping : keyMappings) {
            bool isDown = juce::KeyPress::isKeyCurrentlyDown(mapping.keyCode);






            bool wasDown = false;
            auto it = std::find(activeKeys.begin(), activeKeys.end(), mapping.keyCode);
            wasDown = (it != activeKeys.end());

            if (isDown && !wasDown) {

                int note = baseNote + mapping.noteOffset;
                if (note >= 0 && note <= 127) {
                    callback(juce::MidiMessage::noteOn(1, note, velocity));
                    activeKeys.push_back(mapping.keyCode);
                }
            } else if (!isDown && wasDown) {

                int note = baseNote + mapping.noteOffset;
                if (note >= 0 && note <= 127) {
                    callback(juce::MidiMessage::noteOff(1, note, 0.0f));
                    activeKeys.erase(it);
                }
            }
        }
    }

}
