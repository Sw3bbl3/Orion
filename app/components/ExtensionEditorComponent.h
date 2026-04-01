#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <optional>

#include "orion/Logger.h"
#include "../LuaLanguageServerManager.h"

namespace Orion {

class ExtensionEditorComponent : public juce::Component,
                                 public juce::FileBrowserListener
{
public:
    ExtensionEditorComponent();
    ~ExtensionEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    void loadExtension(const juce::File& extensionDir);
    void loadFile(const juce::File& file);

    void selectionChanged() override;
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) override;
    void fileDoubleClicked(const juce::File& file) override;
    void browserRootChanged(const juce::File&) override {}

private:
    struct ProblemItem {
        juce::String uri;
        int line = 0;
        int character = 0;
        int severity = 1;
        juce::String message;
        juce::String source;
    };

    class ApiListModel : public juce::ListBoxModel {
    public:
        explicit ApiListModel(ExtensionEditorComponent& o) : owner(o) {}
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    private:
        ExtensionEditorComponent& owner;
    };

    class ProblemListModel : public juce::ListBoxModel {
    public:
        explicit ProblemListModel(ExtensionEditorComponent& o) : owner(o) {}
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    private:
        ExtensionEditorComponent& owner;
    };

    void runScript();
    void saveScript();
    void clearLog();
    void createNewFile();
    void createNewFolder();
    void deleteSelected();

    void updateTheme();
    void addLogMessage(const LogMessage& lm);

    void onLuaDiagnostics(const std::vector<LuaDiagnostic>& diags);
    void refreshApiEntries();
    void showApiEntry(int index);
    void sendDictionaryForSymbol(const juce::String& symbol);

    void handleWebEvent(const juce::var& payload);
    void sendDocumentToWeb();
    void sendApiIndexToWeb();
    void sendThemeToWeb();
    void sendProblemsToWeb();
    void emitBackendEvent(const juce::var& payload);

    std::optional<juce::WebBrowserComponent::Resource> getWebResource(const juce::String& path);

    void openApiToolWindow();
    void openDictionaryToolWindow();
    void openProblemsToolWindow();

    juce::TimeSliceThread dirThread;
    std::unique_ptr<juce::DirectoryContentsList> dirContents;
    std::unique_ptr<juce::FileTreeComponent> fileTree;

    ApiListModel apiListModel;
    juce::ListBox apiList;

    ProblemListModel problemListModel;
    juce::ListBox problemList;

    juce::TabbedComponent bottomTabs { juce::TabbedButtonBar::TabsAtTop };
    juce::TextEditor outputLog;
    juce::TextEditor dictionaryView;

    juce::Label filesLabel;
    juce::Label apiLabel;
    juce::Label dictionaryLabel;

    juce::ShapeButton runBtn, saveBtn, clearLogBtn;
    juce::ShapeButton newFileBtn, newFolderBtn, deleteItemBtn;

    juce::TextButton popApiBtn;
    juce::TextButton popDictBtn;
    juce::TextButton popProblemsBtn;

    juce::Label currentFileLabel;

   #if JUCE_WEB_BROWSER
    std::unique_ptr<juce::WebBrowserComponent> webEditor;
    bool webEditorReady = false;
   #else
    juce::CodeDocument fallbackDocument;
    std::unique_ptr<juce::CodeTokeniser> fallbackTokeniser;
    std::unique_ptr<juce::CodeEditorComponent> fallbackEditor;
   #endif

    LuaLanguageServerManager languageServer;

    juce::File currentExtensionDir;
    juce::File currentFile;
    juce::String currentContent;

    std::vector<ProblemItem> problems;
    std::vector<juce::String> apiSymbols;

    std::unique_ptr<juce::DocumentWindow> apiToolWindow;
    std::unique_ptr<juce::DocumentWindow> dictionaryToolWindow;
    std::unique_ptr<juce::DocumentWindow> problemsToolWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExtensionEditorComponent)
};

} // namespace Orion
