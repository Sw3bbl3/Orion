#pragma once

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MarkdownViewer.h"
#include <map>
#include <memory>

namespace Orion {

    class DocumentationViewComponent : public juce::Component, public juce::ListBoxModel
    {
    public:
        DocumentationViewComponent();
        ~DocumentationViewComponent() override;

        void resized() override;
        void paint(juce::Graphics& g) override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;

    private:
        struct DocItem {
            juce::String title;
            juce::String snippet;
            juce::File file;
            bool isCategory = false;
            int level = 0;
            bool expanded = true;
        };

        void scanDocumentation();
        void updateList();
        void toggleSidebar();

        juce::ListBox list;
        MarkdownViewer viewer;
        juce::Label headerLabel;
        juce::Label subtitleLabel;
        juce::Label currentDocLabel;
        juce::TextEditor searchBox;
        
        juce::DrawableButton toggleSidebarBtn { "toggle", juce::DrawableButton::ImageFitted };
        juce::Component resizer;
        std::unique_ptr<juce::Drawable> toggleIcon;
        
        bool sidebarVisible = true;
        int sidebarWidth = 290;
        int startSidebarWidth = 290;

        std::vector<DocItem> items;
        std::map<juce::String, std::vector<juce::File>> docMap;
        std::map<juce::String, juce::String> fileContentCache;
        std::map<juce::String, bool> categoryExpanded;
        juce::String filterText;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentationViewComponent)
    };
}
