#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ExtensionEditorComponent.h"
#include "../ExtensionManager.h"
#include "../../include/orion/SettingsManager.h"

namespace Orion {

    class ExtensionViewComponent : public juce::Component, public juce::ListBoxModel
    {
    public:
        ExtensionViewComponent();
        ~ExtensionViewComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void lookAndFeelChanged() override;

        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;

    private:
        void createNew();
        void refreshList();
        void importExtension();
        void exportExtension();
        void deleteSelectedExtension();
        void updateIconColours();

        juce::ListBox list;
        ExtensionEditorComponent editor;
        juce::ShapeButton newBtn;
        juce::ShapeButton refreshBtn;
        juce::ShapeButton importBtn;
        juce::ShapeButton exportBtn;
        juce::ShapeButton deleteBtn;
        juce::Label headerLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExtensionViewComponent)
    };

}
