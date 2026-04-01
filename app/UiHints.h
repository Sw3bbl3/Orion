#pragma once
#include <JuceHeader.h>
#include "orion/SettingsManager.h"

namespace Orion {

struct UiHints {
    static juce::String withShortcut(const juce::String& text,
                                     int commandId,
                                     juce::ApplicationCommandManager& manager)
    {
        if (!SettingsManager::get().hintPolicy.showShortcutsInHints)
            return text;

        auto* mappings = manager.getKeyMappings();
        if (mappings == nullptr)
            return text;

        auto keys = mappings->getKeyPressesAssignedToCommand(commandId);
        if (keys.isEmpty())
            return text;

        auto keyText = keys.getReference(0).getTextDescription();
        if (keyText.isEmpty())
            return text;

        return text + " (" + keyText + ")";
    }

    static juce::String withShortcutText(const juce::String& text,
                                         const juce::String& shortcutText)
    {
        if (!SettingsManager::get().hintPolicy.showShortcutsInHints)
            return text;

        if (shortcutText.isEmpty())
            return text;

        return text + " (" + shortcutText + ")";
    }
};

} // namespace Orion
