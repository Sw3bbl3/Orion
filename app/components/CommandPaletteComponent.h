#pragma once

#include <JuceHeader.h>

class CommandPaletteComponent : public juce::Component,
                                public juce::TextEditor::Listener,
                                public juce::ListBoxModel,
                                public juce::KeyListener
{
public:
    struct Entry {
        juce::String id;
        juce::String title;
        juce::String shortcut;
        juce::String keywords;
        std::function<void()> action;
    };

    CommandPaletteComponent();

    void setEntries(std::vector<Entry> newEntries);
    std::function<void()> onClose;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    void textEditorTextChanged(juce::TextEditor&) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;

private:
    juce::TextEditor filterBox;
    juce::ListBox resultList;

    std::vector<Entry> allEntries;
    std::vector<int> filteredIndices;

    void applyFilter();
    void executeRow(int row);
};
