#pragma once

#include <JuceHeader.h>
#include "orion/PluginManager.h"

class OrionPluginListComponent : public juce::Component,
                                 public juce::ListBoxModel,
                                 public juce::Timer
{
public:
    OrionPluginListComponent();
    ~OrionPluginListComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;


    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override;


    void timerCallback() override;

private:
    juce::ComboBox categoryBox;
    juce::ComboBox sortBox;
    juce::ListBox listBox;
    std::vector<Orion::PluginInfo> plugins;
    std::vector<Orion::PluginInfo> visiblePlugins;
    bool isScanning = false;

    void rebuildCategories();
    void applyFiltersAndSort();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrionPluginListComponent)
};
