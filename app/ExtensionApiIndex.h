#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace Orion {

struct ExtensionApiParam {
    juce::String name;
    juce::String type;
    juce::String description;
};

struct ExtensionApiEntry {
    juce::String symbol;
    juce::String signature;
    juce::String summary;
    juce::String returns;
    juce::String example;
    std::vector<ExtensionApiParam> params;
    bool fromLuaStdLib = false;
};

class ExtensionApiIndex {
public:
    static ExtensionApiIndex& getInstance();

    void rebuildFromBindings(const std::vector<juce::String>& symbols);

    const std::vector<ExtensionApiEntry>& getEntries() const { return entries; }

    const ExtensionApiEntry* findEntry(const juce::String& symbol) const;
    const ExtensionApiEntry* findBestMatch(const juce::String& tokenOrSymbol) const;

    juce::String toJsonString() const;
    juce::var toVarArray() const;

private:
    ExtensionApiIndex() = default;

    void ensureCuratedDefaults();
    void writeArtifactToDisk() const;

    std::vector<ExtensionApiEntry> entries;
    bool curatedLoaded = false;
};

} // namespace Orion
