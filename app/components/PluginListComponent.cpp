#include "PluginListComponent.h"
#include "../OrionLookAndFeel.h"
#include <algorithm>

OrionPluginListComponent::OrionPluginListComponent()
{
    addAndMakeVisible(categoryBox);
    addAndMakeVisible(sortBox);
    addAndMakeVisible(listBox);
    listBox.setModel(this);
    listBox.setRowHeight(30);

    categoryBox.addItem("All", 1);
    categoryBox.addItem("Orion", 2);
    categoryBox.addItem("Effects", 3);
    categoryBox.addItem("Instruments", 4);
    categoryBox.addItem("Dynamics", 5);
    categoryBox.addItem("EQ", 6);
    categoryBox.addItem("Time", 7);
    categoryBox.addItem("Modulation", 8);
    categoryBox.setSelectedId(1, juce::dontSendNotification);
    categoryBox.onChange = [this] { applyFiltersAndSort(); };

    sortBox.addItem("Sort: Name", 1);
    sortBox.addItem("Sort: Company", 2);
    sortBox.addItem("Sort: Type", 3);
    sortBox.addItem("Sort: Category", 4);
    sortBox.setSelectedId(1, juce::dontSendNotification);
    sortBox.onChange = [this] { applyFiltersAndSort(); };

    startTimer(500);
    Orion::PluginManager::scanPlugins();
}

OrionPluginListComponent::~OrionPluginListComponent()
{
    listBox.setModel(nullptr);
}

void OrionPluginListComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ListBox::backgroundColourId));
    if (isScanning) {
        g.setColour(findColour(OrionLookAndFeel::accentColourId));
        g.setFont(12.0f);
        g.drawText("Scanning: " + Orion::PluginManager::getCurrentScanningFile(), getLocalBounds().removeFromBottom(20), juce::Justification::centredLeft, true);
    }
}

void OrionPluginListComponent::resized()
{
    auto area = getLocalBounds();
    auto controls = area.removeFromTop(28).reduced(4, 2);
    categoryBox.setBounds(controls.removeFromLeft(170));
    controls.removeFromLeft(6);
    sortBox.setBounds(controls.removeFromLeft(150));
    area.removeFromTop(2);
    if (isScanning) area.removeFromBottom(20);
    listBox.setBounds(area);
}

int OrionPluginListComponent::getNumRows()
{
    return (int)visiblePlugins.size();
}

void OrionPluginListComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= (int)visiblePlugins.size()) return;

    if (rowIsSelected)
        g.fillAll(findColour(juce::ListBox::backgroundColourId).brighter(0.1f));

    g.setColour(findColour(juce::ListBox::textColourId));
    g.setFont(14.0f);

    const auto& p = visiblePlugins[rowNumber];
    g.drawText(p.name, 8, 1, width - 100, height - 2, juce::Justification::centredLeft, true);

    g.setColour(findColour(juce::ListBox::textColourId).withAlpha(0.6f));
    g.setFont(10.0f);
    const auto typeText = (p.type == Orion::PluginType::Instrument) ? "INST" : "FX";
    const auto fmtText = p.format == Orion::PluginFormat::VST3 ? "VST3" : (p.format == Orion::PluginFormat::VST2 ? "VST2" : "INT");
    g.drawText(p.vendor, width - 200, 0, 90, height, juce::Justification::centredRight, false);
    g.drawText(juce::String(typeText) + "  " + juce::String(fmtText), width - 100, 0, 92, height, juce::Justification::centredRight, false);
}

juce::var OrionPluginListComponent::getDragSourceDescription(const juce::SparseSet<int>& selectedRows)
{
    if (selectedRows.isEmpty()) return {};
    int row = selectedRows[0];
    if (row >= (int)visiblePlugins.size()) return {};


    const auto& p = visiblePlugins[row];
    juce::String path = p.path;
    if (p.format == Orion::PluginFormat::Internal || path.isEmpty()) {
        path = "internal:" + p.name;
    }
    return juce::String("Plugin: " + path + "|" + p.subID);
}

void OrionPluginListComponent::rebuildCategories()
{
    juce::ignoreUnused(this);
}

void OrionPluginListComponent::applyFiltersAndSort()
{
    visiblePlugins.clear();
    const auto selectedCategory = categoryBox.getText();

    for (const auto& p : plugins) {
        bool include = true;
        if (selectedCategory == "Orion") include = (p.vendor == "Orion");
        else if (selectedCategory == "Effects") include = (p.type == Orion::PluginType::Effect || p.type == Orion::PluginType::Unknown);
        else if (selectedCategory == "Instruments") include = (p.type == Orion::PluginType::Instrument);
        else if (selectedCategory == "Dynamics") include = (p.category.find("Dynamics") != std::string::npos || p.name == "Compressor" || p.name == "Limiter");
        else if (selectedCategory == "EQ") include = (p.name.find("EQ") != std::string::npos || p.category.find("EQ") != std::string::npos);
        else if (selectedCategory == "Time") include = (p.name == "Delay" || p.name == "Reverb" || p.category.find("Delay") != std::string::npos || p.category.find("Reverb") != std::string::npos);
        else if (selectedCategory == "Modulation") include = (p.category.find("Modulation") != std::string::npos);
        if (include) visiblePlugins.push_back(p);
    }

    const int mode = sortBox.getSelectedId();
    std::sort(visiblePlugins.begin(), visiblePlugins.end(), [mode](const Orion::PluginInfo& a, const Orion::PluginInfo& b) {
        if (mode == 2) {
            if (a.vendor != b.vendor) return a.vendor < b.vendor;
            return a.name < b.name;
        }
        if (mode == 3) {
            if (a.type != b.type) return (int)a.type < (int)b.type;
            return a.name < b.name;
        }
        if (mode == 4) {
            if (a.category != b.category) return a.category < b.category;
            return a.name < b.name;
        }
        return a.name < b.name;
    });

    listBox.updateContent();
    repaint();
}

void OrionPluginListComponent::timerCallback()
{
    bool scanning = Orion::PluginManager::isScanning();

    if (scanning != isScanning) {
        isScanning = scanning;
        repaint();
    }

    if (scanning) {
        repaint();
    }


    static int lastCount = 0;
    int currentCount = Orion::PluginManager::getScannedCount();

    if (currentCount != lastCount || (!scanning && isScanning)) {
        plugins = Orion::PluginManager::getAvailablePlugins();
        applyFiltersAndSort();
        lastCount = (int)plugins.size();
    }
}
