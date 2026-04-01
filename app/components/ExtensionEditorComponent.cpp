#include "ExtensionEditorComponent.h"

#include "../ExtensionManager.h"
#include "../ExtensionApiIndex.h"
#include "../OrionIcons.h"
#include "../OrionLookAndFeel.h"
#include "../../include/orion/SettingsManager.h"

#include <algorithm>
#include <cstddef>

namespace Orion {
namespace {

juce::String colourToHex(juce::Colour c)
{
    return "#" + juce::String::toHexString((int)c.getARGB()).substring(2);
}

juce::File findLocalWorkbenchHtml()
{
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto execDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    std::vector<juce::File> roots = {
        execDir.getChildFile("../../assets/ide/monaco"),
        execDir.getChildFile("../assets/ide/monaco"),
        cwd.getChildFile("../assets/ide/monaco"),
        execDir.getChildFile("assets/ide/monaco"),
        cwd.getChildFile("assets/ide/monaco")
    };

    for (const auto& root : roots)
    {
        auto html = root.getChildFile("workbench.html");
        if (html.existsAsFile())
            return html;
    }

    return {};
}

juce::String fileToFileUrl(const juce::File& file)
{
    auto path = file.getFullPathName().replaceCharacter('\\', '/');
    path = juce::URL::addEscapeChars(path, true);

   #if JUCE_WINDOWS
    if (!path.startsWithChar('/'))
        path = "/" + path;
   #endif

    return "file://" + path;
}

class ToolTextWindow : public juce::DocumentWindow
{
public:
    ToolTextWindow(const juce::String& title, const juce::String& text)
        : juce::DocumentWindow(title, juce::Colours::black, juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);

        auto* editor = new juce::TextEditor();
        editor->setMultiLine(true);
        editor->setReadOnly(true);
        editor->setScrollbarsShown(true);
        editor->setText(text, juce::dontSendNotification);
        setContentOwned(editor, true);

        setResizable(true, false);
        centreWithSize(780, 520);
        setVisible(true);
    }

    void closeButtonPressed() override { setVisible(false); }
};

juce::String buildDictionaryText(const ExtensionApiEntry& entry)
{
    juce::String text;
    text << entry.signature << "\n\n";
    text << entry.summary << "\n\n";

    if (!entry.params.empty())
    {
        text << "Parameters:\n";
        for (const auto& p : entry.params)
            text << "- " << p.name << " (" << p.type << "): " << p.description << "\n";
        text << "\n";
    }

    if (entry.returns.isNotEmpty())
        text << "Returns: " << entry.returns << "\n\n";

    if (entry.example.isNotEmpty())
        text << "Example:\n" << entry.example << "\n";

    return text;
}

} // namespace

int ExtensionEditorComponent::ApiListModel::getNumRows()
{
    return (int)owner.apiSymbols.size();
}

void ExtensionEditorComponent::ApiListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)owner.apiSymbols.size())
        return;

    auto textColour = owner.findColour(OrionLookAndFeel::dashboardTextColourId);
    auto accent = owner.findColour(OrionLookAndFeel::dashboardAccentColourId);

    if (rowIsSelected)
    {
        g.setColour(accent.withAlpha(0.24f));
        g.fillRect(0, 0, width, height);
    }

    g.setColour(rowIsSelected ? textColour : textColour.withAlpha(0.85f));
    g.setFont(juce::FontOptions(12.5f, rowIsSelected ? juce::Font::bold : juce::Font::plain));
    g.drawText(owner.apiSymbols[(size_t)rowNumber], 8, 0, width - 10, height, juce::Justification::centredLeft, true);
}

void ExtensionEditorComponent::ApiListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    owner.showApiEntry(row);
}

int ExtensionEditorComponent::ProblemListModel::getNumRows()
{
    return (int)owner.problems.size();
}

void ExtensionEditorComponent::ProblemListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)owner.problems.size())
        return;

    const auto& p = owner.problems[(size_t)rowNumber];

    auto fg = owner.findColour(OrionLookAndFeel::dashboardTextColourId).withAlpha(0.92f);
    if (p.severity == 1) fg = juce::Colour(0xFFFF6B6B);
    else if (p.severity == 2) fg = juce::Colour(0xFFFFC857);

    if (rowIsSelected)
    {
        g.setColour(owner.findColour(OrionLookAndFeel::dashboardAccentColourId).withAlpha(0.24f));
        g.fillRect(0, 0, width, height);
    }

    auto line = "L" + juce::String(p.line + 1) + ":" + juce::String(p.character + 1) + "  " + p.message;
    g.setColour(fg);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(line, 8, 0, width - 12, height, juce::Justification::centredLeft, true);
}

void ExtensionEditorComponent::ProblemListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= (int)owner.problems.size())
        return;

   #if JUCE_WEB_BROWSER
    if (owner.webEditor != nullptr)
    {
        const auto& p = owner.problems[(size_t)row];
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("type", "revealPosition");
        obj->setProperty("line", p.line);
        obj->setProperty("character", p.character);
        owner.emitBackendEvent(juce::var(obj.get()));
    }
   #endif
}

ExtensionEditorComponent::ExtensionEditorComponent()
    : dirThread("ExtensionScanner"),
      apiListModel(*this),
      problemListModel(*this),
      runBtn("Run", juce::Colours::green, juce::Colours::green, juce::Colours::green),
      saveBtn("Save", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
      clearLogBtn("Clear", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
      newFileBtn("NewFile", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
      newFolderBtn("NewFolder", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
      deleteItemBtn("Delete", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey)
{
    dirThread.startThread(juce::Thread::Priority::normal);

    dirContents = std::make_unique<juce::DirectoryContentsList>(nullptr, dirThread);
    fileTree = std::make_unique<juce::FileTreeComponent>(*dirContents);
    fileTree->addListener(this);
    addAndMakeVisible(fileTree.get());

    filesLabel.setText("FILES", juce::dontSendNotification);
    filesLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(filesLabel);

    apiLabel.setText("API EXPLORER", juce::dontSendNotification);
    apiLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(apiLabel);

    dictionaryLabel.setText("DICTIONARY", juce::dontSendNotification);
    dictionaryLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(dictionaryLabel);

    apiList.setModel(&apiListModel);
    apiList.setRowHeight(24);
    addAndMakeVisible(apiList);

    problemList.setModel(&problemListModel);
    problemList.setRowHeight(22);

    outputLog.setMultiLine(true);
    outputLog.setReadOnly(true);
    outputLog.setScrollbarsShown(true);
    outputLog.setFont(juce::FontOptions("Consolas", 12.0f, juce::Font::plain));

    dictionaryView.setMultiLine(true);
    dictionaryView.setReadOnly(true);
    dictionaryView.setScrollbarsShown(true);
    dictionaryView.setFont(juce::FontOptions("Consolas", 12.0f, juce::Font::plain));
    addAndMakeVisible(dictionaryView);

    bottomTabs.addTab("Problems", juce::Colours::transparentBlack, &problemList, false);
    bottomTabs.addTab("Output", juce::Colours::transparentBlack, &outputLog, false);
    addAndMakeVisible(bottomTabs);

    auto setupIconBtn = [this](juce::ShapeButton& b, juce::Path p, std::function<void()> cb) {
        b.setShape(p, true, true, false);
        b.onClick = cb;
        addAndMakeVisible(b);
    };

    setupIconBtn(runBtn, OrionIcons::getPlayIcon(), [this] { runScript(); });
    setupIconBtn(saveBtn, OrionIcons::getSaveIcon(), [this] { saveScript(); });
    setupIconBtn(clearLogBtn, OrionIcons::getTrashIcon(), [this] { clearLog(); });
    setupIconBtn(newFileBtn, OrionIcons::getPlusIcon(), [this] { createNewFile(); });
    setupIconBtn(newFolderBtn, OrionIcons::getFolderIcon(), [this] { createNewFolder(); });
    setupIconBtn(deleteItemBtn, OrionIcons::getTrashIcon(), [this] { deleteSelected(); });

    addAndMakeVisible(currentFileLabel);
    currentFileLabel.setFont(juce::FontOptions(12.0f));
    currentFileLabel.setText("No file open", juce::dontSendNotification);

    popApiBtn.setButtonText("API Window");
    popApiBtn.onClick = [this] { openApiToolWindow(); };
    addAndMakeVisible(popApiBtn);

    popDictBtn.setButtonText("Dictionary Window");
    popDictBtn.onClick = [this] { openDictionaryToolWindow(); };
    addAndMakeVisible(popDictBtn);

    popProblemsBtn.setButtonText("Problems Window");
    popProblemsBtn.onClick = [this] { openProblemsToolWindow(); };
    addAndMakeVisible(popProblemsBtn);

   #if JUCE_WEB_BROWSER
    juce::Component::SafePointer<ExtensionEditorComponent> safeThis(this);
    auto options = juce::WebBrowserComponent::Options()
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withEventListener("orionFrontend", [safeThis](juce::var payload) {
            if (safeThis != nullptr)
                safeThis->handleWebEvent(payload);
        });

   #if JUCE_WINDOWS && JUCE_USE_WIN_WEBVIEW2
    options = options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2);
   #endif

   #if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    options = options.withResourceProvider([safeThis](const juce::String& path) -> std::optional<juce::WebBrowserComponent::Resource> {
        if (safeThis == nullptr)
            return std::nullopt;
        return safeThis->getWebResource(path);
    });
   #endif

    webEditor = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(webEditor.get());

    auto workbenchFile = findLocalWorkbenchHtml();
    if (workbenchFile.existsAsFile())
        webEditor->goToURL(fileToFileUrl(workbenchFile));
   #if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    else
        webEditor->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
   #else
    else
        webEditor->goToURL("about:blank");
   #endif
   #else
    fallbackTokeniser = std::make_unique<juce::LuaTokeniser>();
    fallbackEditor = std::make_unique<juce::CodeEditorComponent>(fallbackDocument, fallbackTokeniser.get());
    addAndMakeVisible(fallbackEditor.get());
   #endif

    juce::Component::SafePointer<ExtensionEditorComponent> safeThisLogs(this);
    Orion::ExtensionManager::getInstance().onLogMessage = [safeThisLogs](const LogMessage& lm) {
        juce::MessageManager::callAsync([safeThisLogs, lm]() {
            if (safeThisLogs != nullptr)
                safeThisLogs->addLogMessage(lm);
        });
    };

    languageServer.onServerLog = [safeThisLogs](const juce::String& msg) {
        juce::MessageManager::callAsync([safeThisLogs, msg]() {
            if (safeThisLogs != nullptr)
                safeThisLogs->outputLog.insertTextAtCaret(msg + "\n");
        });
    };

    languageServer.onDiagnostics = [safeThisLogs](const std::vector<LuaDiagnostic>& diags) {
        if (safeThisLogs != nullptr)
            safeThisLogs->onLuaDiagnostics(diags);
    };

    refreshApiEntries();
    setSize(1024, 680);
    updateTheme();
}

ExtensionEditorComponent::~ExtensionEditorComponent()
{
    Orion::ExtensionManager::getInstance().onLogMessage = nullptr;
    languageServer.stop();

    if (fileTree)
        fileTree->removeListener(this);

    dirThread.stopThread(1000);
}

void ExtensionEditorComponent::paint(juce::Graphics& g)
{
    auto bg = findColour(juce::ResizableWindow::backgroundColourId);
    g.fillAll(bg);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine(48, 0.0f, (float)getWidth());
}

void ExtensionEditorComponent::lookAndFeelChanged()
{
    updateTheme();
}

void ExtensionEditorComponent::updateTheme()
{
    auto textColor = findColour(OrionLookAndFeel::dashboardTextColourId);
    auto accent = findColour(OrionLookAndFeel::dashboardAccentColourId);
    auto bg = findColour(juce::ResizableWindow::backgroundColourId);

    const auto iconCol = textColor.withAlpha(0.6f);
    const auto iconOver = textColor.withAlpha(0.9f);
    runBtn.setColours(juce::Colours::green.withAlpha(0.5f), juce::Colours::green.withAlpha(0.8f), juce::Colours::green);
    saveBtn.setColours(iconCol, iconOver, accent);
    clearLogBtn.setColours(iconCol, iconOver, accent);
    newFileBtn.setColours(iconCol, iconOver, accent);
    newFolderBtn.setColours(iconCol, iconOver, accent);
    deleteItemBtn.setColours(juce::Colours::red.withAlpha(0.5f), juce::Colours::red.withAlpha(0.8f), juce::Colours::red);

    filesLabel.setColour(juce::Label::textColourId, textColor.withAlpha(0.55f));
    apiLabel.setColour(juce::Label::textColourId, textColor.withAlpha(0.55f));
    dictionaryLabel.setColour(juce::Label::textColourId, textColor.withAlpha(0.55f));
    currentFileLabel.setColour(juce::Label::textColourId, textColor.withAlpha(0.75f));

    apiList.setColour(juce::ListBox::backgroundColourId, bg.darker(0.16f));
    problemList.setColour(juce::ListBox::backgroundColourId, bg.darker(0.16f));
    outputLog.setColour(juce::TextEditor::backgroundColourId, bg.darker(0.16f));
    outputLog.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.9f));
    dictionaryView.setColour(juce::TextEditor::backgroundColourId, bg.darker(0.12f));
    dictionaryView.setColour(juce::TextEditor::textColourId, juce::Colours::white.withAlpha(0.9f));

    sendThemeToWeb();
}

void ExtensionEditorComponent::resized()
{
    auto area = getLocalBounds();
    auto toolbar = area.removeFromTop(48).reduced(6, 6);

    runBtn.setBounds(toolbar.removeFromLeft(32));
    saveBtn.setBounds(toolbar.removeFromLeft(32));
    clearLogBtn.setBounds(toolbar.removeFromLeft(32));
    toolbar.removeFromLeft(10);
    currentFileLabel.setBounds(toolbar.removeFromLeft(260));

    popProblemsBtn.setBounds(toolbar.removeFromRight(120));
    toolbar.removeFromRight(6);
    popDictBtn.setBounds(toolbar.removeFromRight(120));
    toolbar.removeFromRight(6);
    popApiBtn.setBounds(toolbar.removeFromRight(100));

    const auto& s = Orion::SettingsManager::get();
    int bottomHeight = s.extensionIdeShowProblems ? juce::jlimit(120, getHeight() / 2, (int)(getHeight() * 0.30f)) : 0;
    auto bottom = area.removeFromBottom(bottomHeight);

    int leftWidth = s.extensionIdeShowApiExplorer ? 300 : 220;
    auto left = area.removeFromLeft(leftWidth);

    int rightWidth = s.extensionIdeShowDictionary ? 300 : 0;
    auto right = area.removeFromRight(rightWidth);

    filesLabel.setBounds(left.removeFromTop(22));
    auto fileToolbar = left.removeFromTop(32);
    const int sBtnW = 28;
    deleteItemBtn.setBounds(fileToolbar.removeFromRight(sBtnW).reduced(4));
    newFolderBtn.setBounds(fileToolbar.removeFromRight(sBtnW).reduced(4));
    newFileBtn.setBounds(fileToolbar.removeFromRight(sBtnW).reduced(4));

    if (s.extensionIdeShowApiExplorer)
    {
        auto split = left.proportionOfHeight(0.56f);
        fileTree->setBounds(left.removeFromTop(split).reduced(2));
        apiLabel.setBounds(left.removeFromTop(24));
        apiList.setVisible(true);
        apiList.setBounds(left.reduced(2));
    }
    else
    {
        fileTree->setBounds(left.reduced(2));
        apiLabel.setBounds(0, 0, 0, 0);
        apiList.setVisible(false);
    }

    if (s.extensionIdeShowDictionary)
    {
        dictionaryLabel.setVisible(true);
        dictionaryView.setVisible(true);
        dictionaryLabel.setBounds(right.removeFromTop(24));
        dictionaryView.setBounds(right.reduced(2));
    }
    else
    {
        dictionaryLabel.setVisible(false);
        dictionaryView.setVisible(false);
    }

   #if JUCE_WEB_BROWSER
    if (webEditor != nullptr)
        webEditor->setBounds(area.reduced(2));
   #else
    if (fallbackEditor != nullptr)
        fallbackEditor->setBounds(area.reduced(2));
   #endif

    bottomTabs.setVisible(s.extensionIdeShowProblems);
    if (s.extensionIdeShowProblems)
        bottomTabs.setBounds(bottom.reduced(2));
}

void ExtensionEditorComponent::loadExtension(const juce::File& extensionDir)
{
    currentExtensionDir = extensionDir;
    dirContents->setDirectory(extensionDir, true, true);

    languageServer.setWorkspace(extensionDir);
    languageServer.start();

    loadFile(extensionDir.getChildFile("main.lua"));
}

void ExtensionEditorComponent::loadFile(const juce::File& file)
{
    currentFile = file;
    if (file.existsAsFile())
    {
        currentContent = file.loadFileAsString();
        currentFileLabel.setText(file.getFileName(), juce::dontSendNotification);
        languageServer.didOpen(file, currentContent);
        outputLog.insertTextAtCaret("Loaded: " + file.getFileName() + "\n");
    }
    else
    {
        currentContent.clear();
        currentFileLabel.setText("New file: " + file.getFileName(), juce::dontSendNotification);
    }

   #if JUCE_WEB_BROWSER
    sendDocumentToWeb();
   #else
    fallbackDocument.replaceAllContent(currentContent);
   #endif
}

void ExtensionEditorComponent::runScript()
{
    saveScript();
    Orion::ExtensionManager::getInstance().log("--- Running ---", LogLevel::Info);
    if (Orion::ExtensionManager::getInstance().runCode(currentContent.toStdString()))
        Orion::ExtensionManager::getInstance().log("--- Done ---", LogLevel::Success);
    else
        Orion::ExtensionManager::getInstance().log("--- Failed ---", LogLevel::Error);
}

void ExtensionEditorComponent::saveScript()
{
    if (currentFile.exists())
    {
        currentFile.replaceWithText(currentContent);
        languageServer.didSave(currentFile);
        Orion::ExtensionManager::getInstance().log("Saved: " + currentFile.getFileName().toStdString(), LogLevel::Info);
        return;
    }

    auto chooser = std::make_shared<juce::FileChooser>(
        "Save Script",
        currentExtensionDir.exists() ? currentExtensionDir
                                     : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.lua");

    juce::Component::SafePointer<ExtensionEditorComponent> safeThis(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, chooser](const juce::FileChooser& fc)
        {
            if (safeThis == nullptr)
                return;

            auto result = fc.getResult();
            if (result == juce::File())
                return;

            safeThis->currentFile = result;
            safeThis->currentFile.replaceWithText(safeThis->currentContent);
            safeThis->currentFileLabel.setText(result.getFileName(), juce::dontSendNotification);
            safeThis->dirContents->refresh();
            safeThis->languageServer.didSave(result);
        });
}

void ExtensionEditorComponent::clearLog() { outputLog.clear(); }

void ExtensionEditorComponent::addLogMessage(const LogMessage& lm)
{
    outputLog.moveCaretToEnd();
    outputLog.insertTextAtCaret("[" + juce::String(lm.timestamp) + "] " + juce::String(lm.message) + "\n");

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "outputLog");
    obj->setProperty("message", juce::String(lm.message));
    emitBackendEvent(juce::var(obj.get()));
}

void ExtensionEditorComponent::onLuaDiagnostics(const std::vector<LuaDiagnostic>& diags)
{
    problems.clear();
    for (const auto& d : diags)
    {
        ProblemItem p;
        p.uri = d.uri;
        p.line = d.line;
        p.character = d.character;
        p.severity = d.severity;
        p.message = d.message;
        p.source = d.source;
        problems.push_back(p);
    }

    problemList.updateContent();
    problemList.repaint();
    sendProblemsToWeb();
}

void ExtensionEditorComponent::refreshApiEntries()
{
    apiSymbols.clear();
    const auto& entries = ExtensionApiIndex::getInstance().getEntries();
    for (const auto& e : entries)
        apiSymbols.push_back(e.symbol);
    apiList.updateContent();
    apiList.repaint();
    sendApiIndexToWeb();
}

void ExtensionEditorComponent::showApiEntry(int index)
{
    if (index < 0 || index >= (int)apiSymbols.size())
        return;
    sendDictionaryForSymbol(apiSymbols[(size_t)index]);
}

void ExtensionEditorComponent::sendDictionaryForSymbol(const juce::String& symbol)
{
    auto* entry = ExtensionApiIndex::getInstance().findBestMatch(symbol);
    if (entry == nullptr)
    {
        dictionaryView.setText(symbol.isEmpty() ? juce::String() : ("No dictionary entry found for: " + symbol), juce::dontSendNotification);
        return;
    }

    dictionaryView.setText(buildDictionaryText(*entry), juce::dontSendNotification);

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "dictionary");
    obj->setProperty("symbol", entry->symbol);
    obj->setProperty("signature", entry->signature);
    obj->setProperty("summary", entry->summary);
    obj->setProperty("returns", entry->returns);
    obj->setProperty("example", entry->example);
    emitBackendEvent(juce::var(obj.get()));
}

void ExtensionEditorComponent::selectionChanged() {}

void ExtensionEditorComponent::fileClicked(const juce::File& file, const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addItem(1, "New File");
        m.addItem(2, "New Folder");
        m.addSeparator();
        m.addItem(3, "Delete");

        juce::Component::SafePointer<ExtensionEditorComponent> safeThis(this);
        m.showMenuAsync(juce::PopupMenu::Options(), [safeThis](int id)
        {
            if (safeThis == nullptr)
                return;
            if (id == 1) safeThis->createNewFile();
            else if (id == 2) safeThis->createNewFolder();
            else if (id == 3) safeThis->deleteSelected();
        });
        return;
    }

    if (file.existsAsFile())
        loadFile(file);
}

void ExtensionEditorComponent::fileDoubleClicked(const juce::File&) {}
void ExtensionEditorComponent::createNewFile()
{
    auto w = std::make_shared<juce::AlertWindow>("New File", "Enter file name (e.g. utils.lua):", juce::AlertWindow::NoIcon);
    w->addTextEditor("name", "script.lua");
    w->addButton("Create", 1);
    w->addButton("Cancel", 0);

    juce::Component::SafePointer<ExtensionEditorComponent> safeThis(this);
    w->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, w](int res)
    {
        if (res != 1 || safeThis == nullptr)
            return;

        auto name = w->getTextEditorContents("name");
        auto f = safeThis->currentExtensionDir.getChildFile(name);
        if (!f.exists())
        {
            f.create();
            safeThis->dirContents->refresh();
            safeThis->loadFile(f);
        }
    }));
}

void ExtensionEditorComponent::createNewFolder()
{
    auto w = std::make_shared<juce::AlertWindow>("New Folder", "Enter folder name:", juce::AlertWindow::NoIcon);
    w->addTextEditor("name", "NewFolder");
    w->addButton("Create", 1);
    w->addButton("Cancel", 0);

    juce::Component::SafePointer<ExtensionEditorComponent> safeThis(this);
    w->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, w](int res)
    {
        if (res != 1 || safeThis == nullptr)
            return;

        auto name = w->getTextEditorContents("name");
        safeThis->currentExtensionDir.getChildFile(name).createDirectory();
        safeThis->dirContents->refresh();
    }));
}

void ExtensionEditorComponent::deleteSelected()
{
    auto file = fileTree->getSelectedFile();
    if (!file.exists())
        return;

    const bool isDir = file.isDirectory();
    const juce::String msg = "Are you sure you want to delete " + file.getFileName() + "?";

    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                                       "Delete",
                                       msg,
                                       "Delete",
                                       "Cancel",
                                       this,
                                       juce::ModalCallbackFunction::create([this, file, isDir](int res)
                                       {
                                           if (res == 0)
                                               return;

                                           if (isDir) file.deleteRecursively();
                                           else file.deleteFile();

                                           if (file == currentFile)
                                           {
                                               currentFile = juce::File();
                                               currentContent.clear();
                                               currentFileLabel.setText("No file open", juce::dontSendNotification);
                                               sendDocumentToWeb();
                                           }

                                           if (dirContents != nullptr)
                                               dirContents->refresh();
                                       }));
}

void ExtensionEditorComponent::handleWebEvent(const juce::var& payload)
{
    auto* obj = payload.getDynamicObject();
    if (obj == nullptr)
        return;

    auto type = obj->getProperty("type").toString();

    if (type == "ready")
    {
        webEditorReady = true;
        sendThemeToWeb();
        sendApiIndexToWeb();
        sendDocumentToWeb();
        sendProblemsToWeb();
        return;
    }

    if (type == "contentChanged")
    {
        currentContent = obj->getProperty("content").toString();
        if (currentFile.existsAsFile())
            languageServer.didChange(currentFile, currentContent);
        return;
    }

    if (type == "saveRequested") { saveScript(); return; }
    if (type == "runRequested") { runScript(); return; }
    if (type == "hoverSymbol") { sendDictionaryForSymbol(obj->getProperty("symbol").toString()); return; }
    if (type == "requestApiIndex") { sendApiIndexToWeb(); return; }
}

void ExtensionEditorComponent::sendDocumentToWeb()
{
   #if JUCE_WEB_BROWSER
    if (!webEditorReady) return;
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "setDocument");
    obj->setProperty("path", currentFile.getFullPathName());
    obj->setProperty("name", currentFile.getFileName());
    obj->setProperty("content", currentContent);
    emitBackendEvent(juce::var(obj.get()));
   #endif
}

void ExtensionEditorComponent::sendApiIndexToWeb()
{
   #if JUCE_WEB_BROWSER
    if (!webEditorReady) return;
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "apiIndex");
    obj->setProperty("entries", ExtensionApiIndex::getInstance().toVarArray());
    emitBackendEvent(juce::var(obj.get()));
   #endif
}

void ExtensionEditorComponent::sendThemeToWeb()
{
   #if JUCE_WEB_BROWSER
    if (!webEditorReady) return;
    auto bg = findColour(juce::ResizableWindow::backgroundColourId);
    auto text = findColour(OrionLookAndFeel::dashboardTextColourId);
    auto accent = findColour(OrionLookAndFeel::dashboardAccentColourId);
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("type", "theme");
    obj->setProperty("background", colourToHex(bg));
    obj->setProperty("text", colourToHex(text));
    obj->setProperty("accent", colourToHex(accent));
    emitBackendEvent(juce::var(obj.get()));
   #endif
}

void ExtensionEditorComponent::sendProblemsToWeb()
{
   #if JUCE_WEB_BROWSER
    if (!webEditorReady) return;
    juce::Array<juce::var> list;
    for (const auto& p : problems)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("uri", p.uri);
        obj->setProperty("line", p.line);
        obj->setProperty("character", p.character);
        obj->setProperty("severity", p.severity);
        obj->setProperty("message", p.message);
        list.add(juce::var(obj.get()));
    }
    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("type", "diagnostics");
    payload->setProperty("items", juce::var(list));
    emitBackendEvent(juce::var(payload.get()));
   #endif
}

void ExtensionEditorComponent::emitBackendEvent(const juce::var& payload)
{
   #if JUCE_WEB_BROWSER
    if (webEditor != nullptr)
        webEditor->emitEventIfBrowserIsVisible("orionBackend", payload);
   #else
    juce::ignoreUnused(payload);
   #endif
}

std::optional<juce::WebBrowserComponent::Resource> ExtensionEditorComponent::getWebResource(const juce::String& path)
{
    auto rel = path;
    if (rel.isEmpty() || rel == "/") rel = "workbench.html";
    if (rel.startsWithChar('/')) rel = rel.substring(1);
    rel = rel.upToFirstOccurrenceOf("?", false, false);

    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto execDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    std::vector<juce::File> roots = {
        execDir.getChildFile("../../assets/ide/monaco"),
        execDir.getChildFile("../assets/ide/monaco"),
        cwd.getChildFile("../assets/ide/monaco"),
        execDir.getChildFile("assets/ide/monaco"),
        cwd.getChildFile("assets/ide/monaco")
    };

    juce::File target;
    for (const auto& root : roots)
    {
        auto candidate = root.getChildFile(rel);
        if (candidate.existsAsFile()) { target = candidate; break; }
    }

    if (!target.existsAsFile()) return std::nullopt;

    juce::MemoryBlock data;
    if (!target.loadFileAsData(data)) return std::nullopt;

    juce::String mime = "text/plain";
    auto ext = target.getFileExtension().toLowerCase();
    if (ext == ".html") mime = "text/html";
    else if (ext == ".css") mime = "text/css";
    else if (ext == ".js") mime = "application/javascript";
    else if (ext == ".json") mime = "application/json";

    juce::WebBrowserComponent::Resource res;
    res.mimeType = mime;
    res.data.assign(reinterpret_cast<const std::byte*>(data.getData()),
                    reinterpret_cast<const std::byte*>(data.getData()) + data.getSize());
    return res;
}

void ExtensionEditorComponent::openApiToolWindow()
{
    auto content = ExtensionApiIndex::getInstance().toJsonString();
    if (apiToolWindow == nullptr) apiToolWindow = std::make_unique<ToolTextWindow>("Orion API Explorer", content);
    else { if (auto* ed = dynamic_cast<juce::TextEditor*>(apiToolWindow->getContentComponent())) ed->setText(content, juce::dontSendNotification); apiToolWindow->setVisible(true); apiToolWindow->toFront(true); }
}

void ExtensionEditorComponent::openDictionaryToolWindow()
{
    auto content = dictionaryView.getText();
    if (dictionaryToolWindow == nullptr) dictionaryToolWindow = std::make_unique<ToolTextWindow>("Dictionary", content);
    else { if (auto* ed = dynamic_cast<juce::TextEditor*>(dictionaryToolWindow->getContentComponent())) ed->setText(content, juce::dontSendNotification); dictionaryToolWindow->setVisible(true); dictionaryToolWindow->toFront(true); }
}

void ExtensionEditorComponent::openProblemsToolWindow()
{
    juce::String content;
    for (const auto& p : problems)
        content << "L" << (p.line + 1) << ":" << (p.character + 1) << " - " << p.message << "\n";
    if (content.isEmpty()) content = "No problems.";

    if (problemsToolWindow == nullptr) problemsToolWindow = std::make_unique<ToolTextWindow>("Problems", content);
    else { if (auto* ed = dynamic_cast<juce::TextEditor*>(problemsToolWindow->getContentComponent())) ed->setText(content, juce::dontSendNotification); problemsToolWindow->setVisible(true); problemsToolWindow->toFront(true); }
}

} // namespace Orion
