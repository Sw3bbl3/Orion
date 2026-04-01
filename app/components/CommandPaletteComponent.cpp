#include "CommandPaletteComponent.h"

CommandPaletteComponent::CommandPaletteComponent()
{
    addAndMakeVisible(filterBox);
    filterBox.setTextToShowWhenEmpty("Type a command...", juce::Colours::grey);
    filterBox.addListener(this);
    filterBox.addKeyListener(this);

    addAndMakeVisible(resultList);
    resultList.setModel(this);
    resultList.addKeyListener(this);
    resultList.setRowHeight(28);

    addKeyListener(this);
    setWantsKeyboardFocus(true);
}

void CommandPaletteComponent::setEntries(std::vector<Entry> newEntries)
{
    allEntries = std::move(newEntries);
    applyFilter();
}

void CommandPaletteComponent::applyFilter()
{
    const auto filter = filterBox.getText().trim().toLowerCase();
    filteredIndices.clear();

    for (int i = 0; i < (int)allEntries.size(); ++i)
    {
        const auto hay = (allEntries[(size_t)i].title + " " + allEntries[(size_t)i].keywords + " " + allEntries[(size_t)i].shortcut).toLowerCase();
        if (filter.isEmpty() || hay.contains(filter))
            filteredIndices.push_back(i);
    }

    resultList.updateContent();
    if (!filteredIndices.empty())
        resultList.selectRow(0);
}

int CommandPaletteComponent::getNumRows()
{
    return (int)filteredIndices.size();
}

void CommandPaletteComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)filteredIndices.size())
        return;

    const auto& entry = allEntries[(size_t)filteredIndices[(size_t)rowNumber]];

    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xFF0A84FF).withAlpha(0.25f));
        g.fillRect(0, 0, width, height);
    }

    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
    g.drawText(entry.title, 8, 0, width - 120, height, juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText(entry.shortcut, width - 110, 0, 102, height, juce::Justification::centredRight, true);
}

void CommandPaletteComponent::executeRow(int row)
{
    if (row < 0 || row >= (int)filteredIndices.size())
        return;

    const auto& entry = allEntries[(size_t)filteredIndices[(size_t)row]];
    if (entry.action)
        entry.action();

    if (onClose)
        onClose();
}

void CommandPaletteComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    executeRow(row);
}

void CommandPaletteComponent::textEditorTextChanged(juce::TextEditor&)
{
    applyFilter();
}

bool CommandPaletteComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose)
            onClose();
        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        executeRow(resultList.getSelectedRow());
        return true;
    }

    return false;
}

void CommandPaletteComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.88f));
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.drawRect(getLocalBounds(), 1);
}

void CommandPaletteComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    filterBox.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    resultList.setBounds(area);
}
