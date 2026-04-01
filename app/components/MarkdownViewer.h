#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>

namespace Orion {

    class MarkdownComponent;

    class MarkdownViewer : public juce::Component
    {
    public:
        MarkdownViewer();
        ~MarkdownViewer() override;

        void loadFile(const juce::File& file);
        void loadContent(const juce::String& text);

        void setEmbedded(bool embedded);

        void paint(juce::Graphics& g) override;
        void resized() override;

        std::function<void()> onClose;

    private:
        class MarkdownComponent : public juce::Component
        {
        public:
            MarkdownComponent();
            ~MarkdownComponent() override;

            void setMarkdown(const juce::String& md, const juce::File& baseDir = juce::File());
            void resized() override;

        private:
            juce::OwnedArray<juce::Component> blocks;
            float padding = 20.0f;

            void addTextBlock(const juce::String& mdText);
            void addCodeBlock(const juce::String& code, const juce::String& lang);
            void addImageBlock(const juce::String& alt, const juce::String& url, const juce::File& baseDir);
            void addAlertBlock(const juce::String& type, const juce::String& content);
            void addTableBlock(const juce::StringArray& rows);

            juce::File currentBaseDir;
        };

        juce::Viewport viewport;
        MarkdownComponent contentComp;
        juce::TextButton closeBtn;
        juce::Label titleLabel;
        bool isEmbedded = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownViewer)
    };

}
