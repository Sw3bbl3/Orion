#include "MainComponent.h"
#include "OrionWindow.h"
#include "orion/Logger.h"
#include "orion/ProjectSerializer.h"
#include "orion/EditorCommands.h"
#include "PluginWindowManager.h"
#include "components/WindowWrapper.h"
#include "components/AboutComponent.h"
#include "ExtensionManager.h"
#include "orion/UxTelemetry.h"
#include "orion/UiThemeRegistry.h"
#include "ui/OrionDesignSystem.h"
#include <thread>
#include <filesystem>
#include <cmath>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
    struct ExportOptions {
        juce::String fileName;
        Orion::ExportFormat format = Orion::ExportFormat::Wav;
        Orion::ExportRange range;
        bool includeTail;
        int bitDepth = 24;
        double sampleRate = 44100.0;
        Orion::Mp3BitrateKbps mp3Bitrate = Orion::Mp3BitrateKbps::Kbps320;
    };

    class ExportSettingsComponent : public juce::Component {
    public:
        explicit ExportSettingsComponent(Orion::AudioEngine& engineRef) : engine(engineRef) {
            setupHeader();
            setupSections();

            updateRangeState();
            rangeBox.onChange = [this] { updateRangeState(); };
        }

        ExportOptions getOptions() const {
            auto selectedName = fileNameEditor.getText().trim();
            if (selectedName.isEmpty()) selectedName = "orion-export";

            ExportOptions options;
            options.fileName = selectedName;

            auto fmtId = formatBox.getSelectedId();
            if (fmtId == 1) { options.format = Orion::ExportFormat::Wav; options.bitDepth = 24; }
            else if (fmtId == 2) { options.format = Orion::ExportFormat::Wav; options.bitDepth = 32; }
            else { options.format = Orion::ExportFormat::Mp3; options.bitDepth = 16; }

            options.range = Orion::ExportRange::FullProject;
            if (rangeBox.getSelectedId() == 2) options.range = Orion::ExportRange::LoopRegion;
            if (rangeBox.getSelectedId() == 3) options.range = Orion::ExportRange::FromCursor;

            options.includeTail = includeTailToggle.getToggleState();

            auto srId = sampleRateBox.getSelectedId();
            if (srId == 1) options.sampleRate = engine.getSampleRate();
            else if (srId == 2) options.sampleRate = 44100.0;
            else if (srId == 3) options.sampleRate = 48000.0;
            else if (srId == 4) options.sampleRate = 96000.0;

            switch (bitrateBox.getSelectedId()) {
                case 1: options.mp3Bitrate = Orion::Mp3BitrateKbps::Kbps128; break;
                case 2: options.mp3Bitrate = Orion::Mp3BitrateKbps::Kbps192; break;
                case 3: options.mp3Bitrate = Orion::Mp3BitrateKbps::Kbps256; break;
                default: options.mp3Bitrate = Orion::Mp3BitrateKbps::Kbps320; break;
            }

            return options;
        }

        void paint(juce::Graphics& g) override {
            auto bg = findColour(juce::ResizableWindow::backgroundColourId);
            g.fillAll(bg.darker(0.2f));

            auto area = getLocalBounds().toFloat().reduced(12);
            g.setColour(bg.darker(0.05f));
            g.fillRoundedRectangle(area, 8.0f);
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.drawRoundedRectangle(area, 8.0f, 1.0f);

            // Draw section separators
            g.setColour(juce::Colours::white.withAlpha(0.03f));
            int y1 = 120;
            int y2 = 230;
            g.drawHorizontalLine(y1, 24.0f, (float)getWidth() - 24.0f);
            g.drawHorizontalLine(y2, 24.0f, (float)getWidth() - 24.0f);
        }

        void resized() override {
            auto area = getLocalBounds().reduced(24);

            auto header = area.removeFromTop(60);
            titleLabel.setBounds(header.removeFromTop(32));
            subtitleLabel.setBounds(header);

            area.removeFromTop(15);

            // File section
            auto fileRow = area.removeFromTop(30);
            fileNameLabel.setBounds(fileRow.removeFromLeft(120));
            fileNameEditor.setBounds(fileRow);

            area.removeFromTop(35); // Space for separator and margin

            // Format section
            auto formatRow = area.removeFromTop(30);
            formatLabel.setBounds(formatRow.removeFromLeft(120));
            formatBox.setBounds(formatRow);

            area.removeFromTop(15);
            auto sampleRow = area.removeFromTop(30);
            sampleRateLabel.setBounds(sampleRow.removeFromLeft(120));
            sampleRateBox.setBounds(sampleRow);

            area.removeFromTop(15);
            auto bitrateRow = area.removeFromTop(30);
            bitrateLabel.setBounds(bitrateRow.removeFromLeft(120));
            bitrateBox.setBounds(bitrateRow);

            area.removeFromTop(35); // Space for separator and margin

            // Range section
            auto rangeRow = area.removeFromTop(30);
            rangeLabel.setBounds(rangeRow.removeFromLeft(120));
            rangeBox.setBounds(rangeRow);

            area.removeFromTop(15);
            auto tailRow = area.removeFromTop(30);
            tailLabel.setBounds(tailRow.removeFromLeft(120));
            includeTailToggle.setBounds(tailRow);

            area.removeFromTop(10);
            if (rangeHint != nullptr) {
                rangeHint->setBounds(area.removeFromTop(40));
            }
        }

        void setRangeHintLabel(juce::Label* hint) { rangeHint = hint; updateRangeState(); }

    private:
        Orion::AudioEngine& engine;
        juce::Label titleLabel;
        juce::Label subtitleLabel;

        juce::Label fileNameLabel, formatLabel, rangeLabel, tailLabel, sampleRateLabel;
        juce::Label bitrateLabel;
        juce::TextEditor fileNameEditor;
        juce::ComboBox formatBox, rangeBox, sampleRateBox, bitrateBox;
        juce::ToggleButton includeTailToggle;
        juce::Component::SafePointer<juce::Label> rangeHint;

        void setupHeader() {
            titleLabel.setText("Export Render", juce::dontSendNotification);
            titleLabel.setJustificationType(juce::Justification::centredLeft);
            titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
            addAndMakeVisible(titleLabel);

            subtitleLabel.setText("Configure your final mixdown. Orion uses high-quality 64-bit internal processing.", juce::dontSendNotification);
            subtitleLabel.setJustificationType(juce::Justification::centredLeft);
            subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
            addAndMakeVisible(subtitleLabel);
        }

        void setupSections() {
            setupLabel(fileNameLabel, "File Name");
            fileNameEditor.setText("My Project", juce::dontSendNotification);
            addAndMakeVisible(fileNameEditor);

            setupLabel(formatLabel, "Format");
            formatBox.addItem("WAV (24-bit PCM)", 1);
            formatBox.addItem("WAV (32-bit Float)", 2);
            formatBox.addItem("MP3 (320kbps)", 3);
            formatBox.setSelectedId(1);
            formatBox.onChange = [this] {
                const bool mp3 = formatBox.getSelectedId() == 3;
                bitrateLabel.setVisible(mp3);
                bitrateBox.setVisible(mp3);
                resized();
            };
            addAndMakeVisible(formatBox);

            setupLabel(sampleRateLabel, "Sample Rate");
            sampleRateBox.addItem("Project (Current)", 1);
            sampleRateBox.addItem("44.1 kHz", 2);
            sampleRateBox.addItem("48.0 kHz", 3);
            sampleRateBox.addItem("96.0 kHz", 4);
            sampleRateBox.setSelectedId(1);
            addAndMakeVisible(sampleRateBox);

            setupLabel(bitrateLabel, "MP3 Bitrate");
            bitrateBox.addItem("128 kbps", 1);
            bitrateBox.addItem("192 kbps", 2);
            bitrateBox.addItem("256 kbps", 3);
            bitrateBox.addItem("320 kbps", 4);
            bitrateBox.setSelectedId(4);
            bitrateLabel.setVisible(false);
            bitrateBox.setVisible(false);
            addAndMakeVisible(bitrateBox);

            setupLabel(rangeLabel, "Range");
            rangeBox.addItem("Full Project", 1);
            rangeBox.addItem("Loop Selection", 2);
            rangeBox.addItem("From Cursor to End", 3);
            rangeBox.setSelectedId(1);
            addAndMakeVisible(rangeBox);

            setupLabel(tailLabel, "Options");
            includeTailToggle.setButtonText("Include Effects Tail (2s)");
            includeTailToggle.setToggleState(true, juce::dontSendNotification);
            addAndMakeVisible(includeTailToggle);
        }

        void setupLabel(juce::Label& label, const juce::String& text) {
            label.setText(text, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredLeft);
            label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
            label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            addAndMakeVisible(label);
        }

        void updateRangeState() {
            bool hasLoop = engine.getLoopEnd() > engine.getLoopStart();
            if (rangeBox.getSelectedId() == 2 && !hasLoop) {
                rangeBox.setSelectedId(1, juce::dontSendNotification);
            }

            if (rangeHint != nullptr) {
                juce::String hint;
                switch (rangeBox.getSelectedId()) {
                    case 1: hint = "Full Project: Exports everything from the start to the end of the last clip."; break;
                    case 2: hint = "Loop Selection: Exports the region defined by the loop markers on the ruler."; break;
                    case 3: hint = "From Cursor: Exports from the current playhead position to the end of the project."; break;
                }
                rangeHint->setText(hint, juce::dontSendNotification);
            }
        }
    };

    static bool matchesShortcutSpec(const juce::KeyPress& key, const std::string& specRaw)
    {
        juce::String spec(specRaw);
        spec = spec.trim();
        if (spec.isEmpty())
            return false;

        bool wantCtrl = false;
        bool wantAlt = false;
        bool wantShift = false;
        juce::String keyToken;

        juce::StringArray tokens;
        tokens.addTokens(spec, "+", "");
        tokens.trim();
        tokens.removeEmptyStrings();
        for (auto t : tokens)
        {
            t = t.trim().toLowerCase();
            if (t == "ctrl" || t == "cmd" || t == "command")
                wantCtrl = true;
            else if (t == "alt" || t == "option")
                wantAlt = true;
            else if (t == "shift")
                wantShift = true;
            else
                keyToken = t;
        }

        const bool hasCtrl = key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown();
        const bool hasAlt = key.getModifiers().isAltDown();
        const bool hasShift = key.getModifiers().isShiftDown();

        if (wantCtrl != hasCtrl) return false;
        if (wantAlt != hasAlt) return false;
        if (wantShift != hasShift) return false;
        if (keyToken.isEmpty()) return false;

        if (keyToken == "space") return key.getKeyCode() == juce::KeyPress::spaceKey;
        if (keyToken == "esc" || keyToken == "escape") return key.getKeyCode() == juce::KeyPress::escapeKey;
        if (keyToken == "return" || keyToken == "enter") return key.getKeyCode() == juce::KeyPress::returnKey;
        if (keyToken == "delete") return key.getKeyCode() == juce::KeyPress::deleteKey;
        if (keyToken == "backspace") return key.getKeyCode() == juce::KeyPress::backspaceKey;

        if (keyToken.length() == 1)
        {
            const auto tc = juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
            return tc == keyToken[0];
        }

        return false;
    }
}

MainComponent::MainComponent()
    : commandManager(engine),
      dashboard(projectManager, deviceManager, [this](const juce::String& theme){ setTheme(theme); }),
      header(engine, commandManager),
      timeline(engine, commandHistory),
      inspector(engine),
      editor(engine, commandHistory),
      mixer(engine, commandHistory)
{
    Orion::Logger::info("MainComponent: Constructor Start");
    PluginWindowManager::getInstance().setAudioEngine(&engine);
    Orion::ExtensionManager::getInstance().setContext(&engine, &projectManager);


    auto& em = Orion::ExtensionManager::getInstance();
    em.getSelectedClips = [this]() -> std::vector<Orion::ExtensionManager::SelectedClipInfo> {
        std::vector<Orion::ExtensionManager::SelectedClipInfo> result;
        const auto& sel = timeline.getSelection();
        auto tracks = engine.getTracks();


        for (const auto& clip : sel.clips) {
             for (size_t t = 0; t < tracks.size(); ++t) {
                 if (auto clipsList = tracks[t]->getClips()) {
                     for (size_t c = 0; c < clipsList->size(); ++c) {
                         if ((*clipsList)[c] == clip) {
                             result.push_back({(int)t + 1, (int)c + 1});
                             goto nextClip;
                         }
                     }
                 }
             }
             nextClip:;
        }


        for (const auto& clip : sel.midiClips) {
             for (size_t t = 0; t < tracks.size(); ++t) {
                 if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(tracks[t])) {
                     if (auto clipsList = inst->getMidiClips()) {
                         for (size_t c = 0; c < clipsList->size(); ++c) {
                             if ((*clipsList)[c] == clip) {
                                 result.push_back({(int)t + 1, (int)c + 1});
                                 goto nextMidiClip;
                             }
                         }
                     }
                 }
             }
             nextMidiClip:;
        }
        return result;
    };

    em.getSelectedNotes = [this]() -> std::vector<Orion::ExtensionManager::SelectedNoteInfo> {
        std::vector<Orion::ExtensionManager::SelectedNoteInfo> result;
        auto currentClip = editor.getCurrentClip();
        auto currentTrack = editor.getCurrentTrack();

        if (currentClip && currentTrack) {
            int trackIdx = -1;
            int clipIdx = -1;
            auto tracks = engine.getTracks();
            for(size_t i=0; i<tracks.size(); ++i) {
                if (tracks[i] == currentTrack) {
                    trackIdx = (int)i;
                    if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(tracks[i])) {
                        if (auto clips = inst->getMidiClips()) {
                            for(size_t k=0; k<clips->size(); ++k) {
                                if ((*clips)[k] == currentClip) {
                                    clipIdx = (int)k;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
            }

            if (trackIdx != -1 && clipIdx != -1) {
                const auto& indices = editor.getPianoRoll().getGrid().getSelectedIndices();
                for (int noteIdx : indices) {
                    result.push_back({trackIdx + 1, clipIdx + 1, noteIdx + 1});
                }
            }
        }
        return result;
    };


    Orion::Logger::info("MainComponent: Setting Default LookAndFeel");
    juce::LookAndFeel::setDefaultLookAndFeel(&lnf);


    formatManager.registerBasicFormats();


    auto settings = Orion::SettingsManager::get();
    const auto resolvedThemeId = Orion::UiThemeRegistry::resolveThemeId(settings.themeId.empty() ? settings.theme : settings.themeId);
    const auto resolvedThemeName = Orion::UiThemeRegistry::getDisplayName(resolvedThemeId);
    if (settings.themeId != resolvedThemeId || settings.theme != resolvedThemeName) {
        settings.themeId = resolvedThemeId;
        settings.theme = resolvedThemeName;
        Orion::SettingsManager::set(settings);
    }
    setTheme(settings.theme);

    juce::Desktop::getInstance().setGlobalScaleFactor(settings.uiScale);


    Orion::Logger::info("MainComponent: Creating TitleBar");
    addAndMakeVisible(customTitleBar);
    customTitleBar.setModel(this);

    addAndMakeVisible(statusBar);
    statusBar.setOnToggleInspector([this] {
        if (inspectorWindow) inspectorWindow->setVisible(!inspectorWindow->isVisible());
        else inspectorPanel->setVisible(!inspectorPanel->isVisible());
        resized();
    });
    statusBar.setOnToggleMixer([this] {
        if (mixerWindow) mixerWindow->setVisible(!mixerWindow->isVisible());
        else mixerPanel->setVisible(!mixerPanel->isVisible());
        resized();
    });
    statusBar.setOnTogglePianoRoll([this] {
        if (editorWindow) editorWindow->setVisible(!editorWindow->isVisible());
        else editorPanel->setVisible(!editorPanel->isVisible());
        resized();
    });
    statusBar.setOnToggleBrowser([this] {
        if (browserWindow) browserWindow->setVisible(!browserWindow->isVisible());
        else browserPanel->setVisible(!browserPanel->isVisible());
        resized();
    });

    guidedStartWizard.setState(guidedStartState);
    guidedStartWizard.setVisible(false);


    inspectorPanel = std::make_unique<DockablePanel>("Inspector", inspector,
        [this]{ inspectorPanel->setVisible(false); resized(); },
        [this]{ detachPanel(inspectorPanel.get(), &inspector, &inspectorWindow, "Inspector"); });

    browserPanel = std::make_unique<DockablePanel>("Browser", browser,
        [this]{ browserPanel->setVisible(false); resized(); },
        [this]{ detachPanel(browserPanel.get(), &browser, &browserWindow, "Browser"); });

    mixerPanel = std::make_unique<DockablePanel>("Mixer", mixer,
        [this]{ mixerPanel->setVisible(false); resized(); },
        [this]{ detachPanel(mixerPanel.get(), &mixer, &mixerWindow, "Mixer"); });

    editorPanel = std::make_unique<DockablePanel>("Piano Roll", editor,
        [this]{ editorPanel->setVisible(false); resized(); },
        [this]{ detachPanel(editorPanel.get(), &editor, &editorWindow, "Piano Roll"); });

    extensionsPanel = std::make_unique<DockablePanel>("Extensions", extensionView,
        [this]{ extensionsPanel->setVisible(false); resized(); },
        [this]{ detachPanel(extensionsPanel.get(), &extensionView, &extensionsWindow, "Extensions"); });


    inspectorPanel->setVisible(false);
    browserPanel->setVisible(true);
    mixerPanel->setVisible(true);
    editorPanel->setVisible(false);
    extensionsPanel->setVisible(Orion::SettingsManager::get().developerMode);

    Orion::Logger::info("MainComponent: Adding Child Components");
    addAndMakeVisible(header);
    addAndMakeVisible(timeline);


    header.getCurrentToolId = [this] {
        return (int)timeline.getTool();
    };


    addAndMakeVisible(browserPanel.get());
    addAndMakeVisible(inspectorPanel.get());
    addAndMakeVisible(editorPanel.get());
    addAndMakeVisible(mixerPanel.get());
    addAndMakeVisible(extensionsPanel.get());



    verticalLayout.setItemLayout(0, 50, 50, 50);
    verticalLayout.setItemLayout(1, 200, -1.0, -0.5);
    verticalLayout.setItemLayout(2, 5, 5, 5);
    verticalLayout.setItemLayout(3, 350, -1.0, -0.5);


    horizontalLayout.setItemLayout(0, 240, 450, 260);
    horizontalLayout.setItemLayout(1, 5, 5, 5);
    horizontalLayout.setItemLayout(2, 400, -1.0, -1.0);
    horizontalLayout.setItemLayout(3, 5, 5, 5);
    horizontalLayout.setItemLayout(4, 240, 500, 280);


    resizerV = std::make_unique<juce::StretchableLayoutResizerBar>(&verticalLayout, 2, false);
    addAndMakeVisible(resizerV.get());

    resizerLeft = std::make_unique<juce::StretchableLayoutResizerBar>(&horizontalLayout, 1, true);
    addAndMakeVisible(resizerLeft.get());

    resizerRight = std::make_unique<juce::StretchableLayoutResizerBar>(&horizontalLayout, 3, true);
    addAndMakeVisible(resizerRight.get());


    addAndMakeVisible(dashboard);
    dashboard.onProjectSelected = [this](const std::string& path) {
        if (!loadingWindow) loadingWindow = std::make_unique<LoadingWindow>();
        loadingWindow->show();

        engine.cancelPreviewRender();
        if (previewThread.joinable()) previewThread.join();

        juce::Component::SafePointer<MainComponent> sp(this);
        juce::MessageManager::callAsync([sp, path](){
            if (!sp) return;
            std::string extraData;
            if (Orion::ProjectSerializer::load(sp->engine, path, &extraData)) {
                sp->currentProjectFile = path;
                sp->state = Editor;
                sp->firstPlaybackLogged = false;
                sp->projectManager.addToRecents(path);
                sp->commandHistory.clear();
                sp->commandHistory.setSavePoint();
                sp->applyLayoutState(extraData);
                sp->resized();
                sp->timeline.refresh();
                sp->mixer.refresh();
            } else {
                 juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Failed to load project.");
            }
            if (sp->loadingWindow) sp->loadingWindow->hide();
        });
    };

    dashboard.onNewProjectCreated = [this](const Orion::NewProjectSettings& s) {
        if (!loadingWindow) loadingWindow = std::make_unique<LoadingWindow>();
        loadingWindow->show();

        engine.cancelPreviewRender();
        if (previewThread.joinable()) previewThread.join();

        juce::Component::SafePointer<MainComponent> sp(this);
        juce::MessageManager::callAsync([sp, s](){
            if (!sp) return;
            if (sp->projectManager.createProject(sp->engine, s.name, s.templateName, s.bpm, s.sampleRate, s.author, s.genre, s.path)) {
                 sp->state = Editor;
                 sp->firstPlaybackLogged = false;
                 Orion::UxTelemetry::logEvent(Orion::UxEventType::ProjectCreated, {{"name", s.name}});
                 sp->guidedStartState.lastTrackType = s.quickStartTrackType;
                 sp->guidedStartState.lastBpm = (float) s.bpm;
                 sp->guidedStartState.lastLoopBars = s.quickStartLoopBars;
                 sp->guidedStartState.dismissed = true;

                 std::string root = s.path.empty() ? Orion::SettingsManager::get().rootDataPath + "/Projects" : s.path;
                 sp->currentProjectFile = (std::filesystem::path(root) / s.name / (s.name + ".orion")).string();
                 sp->commandHistory.clear();
                 sp->commandHistory.setSavePoint();


                 if (!Orion::SettingsManager::get().projectLayoutState.empty())
                 {
                     sp->applyLayoutState(Orion::SettingsManager::get().projectLayoutState);
                 }
                 else
                 {
                     sp->editorPanel->setVisible(false);
                     sp->mixerPanel->setVisible(true);
                 }

                 sp->applyGuidedStartSetup(sp->guidedStartState, s.quickStartCreateStarterClip, true);

                 sp->resized();
            } else {
                 juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Failed to create project (Name might exist).");
            }
            if (sp->loadingWindow) sp->loadingWindow->hide();
        });
    };

    dashboard.onProjectPreviewPlay = [this](const std::string& path) {
        dashboardTransport.stop();
        dashboardTransport.setSource(nullptr);

        if (path.empty()) return;

        juce::File file(path);

        if (file.hasFileExtension("orion")) {
            file = file.getParentDirectory().getChildFile("preview.wav");
        }

        if (file.existsAsFile()) {
            auto* reader = formatManager.createReaderFor(file);
            if (reader) {
                dashboardReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                dashboardTransport.setSource(dashboardReaderSource.get(), 0, nullptr, reader->sampleRate);
                dashboardTransport.setGain(1.0f);
                dashboardTransport.start();
            }
        }
    };

    dashboard.onProjectPreviewStop = [this]() {
        dashboardTransport.stop();
    };

    dashboard.onProjectPreviewSeek = [this](double pos) {
        dashboardTransport.setPosition(pos);
    };

    dashboard.onSettingsRequested = [this]() {
        showSettings();
    };


    timeline.onClipSelected = [this](std::shared_ptr<Orion::AudioClip> clip) {
        inspector.inspectClip(clip);
    };

    timeline.onInvokeCommand = [this](int cmdId) {
        commandManager.getApplicationCommandManager().invokeDirectly((juce::CommandID)cmdId, true);
    };
    timeline.onShowGuidedStart = [this] {
        showGuidedStartWizard(true);
    };

    auto setSelectedTrack = [this](std::shared_ptr<Orion::AudioTrack> track) {
        inspector.inspectTrack(track);
        timeline.setSelectedTrack(track);
        mixer.setSelectedTrack(track);
    };

    timeline.onTrackSelected = [setSelectedTrack](std::shared_ptr<Orion::AudioTrack> track) {
        setSelectedTrack(track);
    };

    mixer.onTrackSelected = [setSelectedTrack](std::shared_ptr<Orion::AudioTrack> track) {
        setSelectedTrack(track);
    };

    timeline.onMidiClipSelected = [this](std::shared_ptr<Orion::MidiClip> clip, std::shared_ptr<Orion::InstrumentTrack> track, bool mod) {
        juce::ignoreUnused(mod);
        if (editorPanel->isVisible()) {
            editor.showPianoRoll(clip, track);
        }
        updateGuidedStartVisibility();
    };

    timeline.onMidiClipOpened = [this](std::shared_ptr<Orion::MidiClip> clip, std::shared_ptr<Orion::InstrumentTrack> track) {
        editorPanel->setVisible(true);
        resized();
        editor.showPianoRoll(clip, track);
        Orion::UxTelemetry::logEvent(Orion::UxEventType::PianoRollOpened);
        updateGuidedStartVisibility();
    };

    inspector.onClipChanged = [this] {
        timeline.repaint();
    };


    commandManager.registerAllCommands();
    commandManager.getApplicationCommandManager().registerAllCommandsForTarget(this);

    addKeyListener(commandManager.getApplicationCommandManager().getKeyMappings());
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    setSize (1400, 900);
    Orion::Logger::info("MainComponent: Constructor End");


    startTimer(50);
}

void MainComponent::parentHierarchyChanged()
{
    if (auto* w = getTopLevelComponent())
    {
        if (!windowResizer)
        {

            windowResizer = std::make_unique<juce::ResizableBorderComponent>(w, nullptr);
            windowResizer->setBorderThickness(juce::BorderSize<int>(4));
            addAndMakeVisible(windowResizer.get());
        }
    }
}

void MainComponent::timerCallback()
{
    if (state == Dashboard && !dashboardTransport.isPlaying() && dashboardTransport.getCurrentPosition() > 0) {
        // Check if it reached the end (allowing for a small buffer)
        if (dashboardTransport.getCurrentPosition() >= dashboardTransport.getLengthInSeconds() - 0.01) {
            dashboard.setPlaybackPosition(0, true);
            dashboardTransport.setPosition(0);
        }
    }

    if (isFirstRun) {
        switch (initStep)
        {
            case 0:
                if (onInitProgress) onInitProgress(0.2f, "Initializing Audio Engine...");
                initStep++;
                break;
            case 1:
                Orion::Logger::info("MainComponent: Async Audio Initialization");
                if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
                    && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
                {
                    juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                                    [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
                }
                else
                {
                    setAudioChannels (2, 2);
                }
                if (onInitProgress) onInitProgress(0.4f, "Setting Up Audio...");
                initStep++;
                break;
            case 2:
                if (onInitProgress) onInitProgress(0.6f, "Scanning MIDI Devices...");
                initStep++;
                break;
            case 3:
                {
                    {
                        auto midiInputs = juce::MidiInput::getAvailableDevices();
                        for (const auto& input : midiInputs) {
                            if (!deviceManager.isMidiInputDeviceEnabled(input.identifier)) {
                                deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
                            }
                            deviceManager.addMidiInputDeviceCallback(input.identifier, this);
                        }
                    }
                    if (onInitProgress) onInitProgress(0.8f, "Loading Plugins...");

                    bool isScreenshotRun = false;
                    juce::StringArray args;
                    args.addTokens(juce::JUCEApplication::getInstance()->getCommandLineParameters(), " ", "\"'");
                    if (args.contains("--screenshot")) isScreenshotRun = true;

                    Orion::PluginManager::loadCache();
                    if (Orion::SettingsManager::get().scanPluginsAtStartup &&
                        Orion::SettingsManager::get().pluginBackgroundVerificationOnStartup &&
                        !isScreenshotRun) {
                        Orion::PluginManager::scanPlugins(false);
                    }
                    initStep++;
                }
                break;
            case 4:
                if (onInitProgress) onInitProgress(1.0f, "Starting User Interface...");
                initStep++;
                break;
            case 5:
                stopTimer();
                isFirstRun = false;
                if (onInitComplete) onInitComplete();
                startTimerHz(30);
                break;
        }
    } else {

        if (state == Dashboard && dashboardTransport.isPlaying()) {
            dashboard.setPlaybackPosition(dashboardTransport.getCurrentPosition());
        }

        if (commandHistory.isDirty() != lastDirtyState) {
            lastDirtyState = commandHistory.isDirty();
            updateTitleBarForState();
        }

        trackUxEventProgress();
        updateGuidedStartVisibility();
        performAutoSave();
    }
}

void MainComponent::promptToSaveAndClose()
{
    if (commandHistory.isDirty())
    {
        juce::NativeMessageBox::showYesNoCancelBox(juce::AlertWindow::QuestionIcon, "Save Changes?",
            "You have unsaved changes. Do you want to save them before closing?",
            this,
            juce::ModalCallbackFunction::create([this](int result)
            {
                if (result == 1)
                {
                    saveProject([this](bool success){
                        if (success) closeProject();
                    });
                }
                else if (result == 0)
                {
                    closeProject();
                }
            }));
    }
    else
    {
        closeProject();
    }
}

void MainComponent::startAutoSave()
{
    if (!isTimerRunning()) startTimerHz(30);
}

void MainComponent::performAutoSave()
{
    static juce::uint32 lastSaveTime = 0;
    auto now = juce::Time::getMillisecondCounter();
    auto& s = Orion::SettingsManager::get();

    if (!s.autoSaveEnabled || s.autoSaveInterval <= 0) return;

    if (now - lastSaveTime < (juce::uint32)(s.autoSaveInterval * 60 * 1000)) return;

    if (!currentProjectFile.empty() && state == Editor) {
        lastSaveTime = now;
        Orion::Logger::info("Auto-Saving project to: " + currentProjectFile);
        Orion::ProjectSerializer::save(engine, currentProjectFile, getLayoutState());
    }
}

MainComponent::~MainComponent()
{
    Orion::Logger::info("MainComponent: Destructor");
    engine.cancelPreviewRender();
    if (previewThread.joinable()) previewThread.join();

    PluginWindowManager::getInstance().setAudioEngine(nullptr);

    if (inspectorPanel) inspectorPanel->removeChildComponent(&inspector);
    if (browserPanel) browserPanel->removeChildComponent(&browser);
    if (mixerPanel) mixerPanel->removeChildComponent(&mixer);
    if (editorPanel) editorPanel->removeChildComponent(&editor);
    if (extensionsPanel) extensionsPanel->removeChildComponent(&extensionView);

    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    Orion::Logger::info("MainComponent: prepareToPlay (SR: " + std::to_string(sampleRate) + ", Block: " + std::to_string(samplesPerBlockExpected) + ")");
    engine.setSampleRate(sampleRate);
    engine.setBlockSize(samplesPerBlockExpected);

    browserMixBuffer.setSize(2, samplesPerBlockExpected + 256);
    browserMixBuffer.clear();

    dashboardMixBuffer.setSize(2, samplesPerBlockExpected + 256);
    dashboardMixBuffer.clear();

    browser.prepareToPlay(samplesPerBlockExpected, sampleRate);
    dashboardTransport.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    int numSamples = bufferToFill.numSamples;
    int numChannels = bufferToFill.buffer->getNumChannels();

    std::vector<float> interleaved(numSamples * 2, 0.0f);

    if (numChannels > 0) {
        auto* left = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
        auto* right = (numChannels > 1) ? bufferToFill.buffer->getReadPointer(1, bufferToFill.startSample) : nullptr;

        for (int i=0; i<numSamples; ++i) {
            interleaved[i*2] = left[i];
            interleaved[i*2+1] = right ? right[i] : left[i];
        }
    }

    engine.process(interleaved.data(), interleaved.data(), numSamples, 2);

    if (numChannels > 0) {
        auto* left = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
        auto* right = (numChannels > 1) ? bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample) : nullptr;

        for (int i=0; i<numSamples; ++i) {
            left[i] = interleaved[i*2];
            if (right) right[i] = interleaved[i*2+1];
        }
    }

    if (browserMixBuffer.getNumSamples() < numSamples) {
         browserMixBuffer.setSize(2, numSamples);
    }

    juce::AudioSourceChannelInfo browserInfo(&browserMixBuffer, 0, numSamples);
    browserMixBuffer.clear(0, numSamples);
    browser.getNextAudioBlock(browserInfo);

    int maxCh = std::min(numChannels, browserMixBuffer.getNumChannels());

    for (int ch = 0; ch < maxCh; ++ch)
    {
        bufferToFill.buffer->addFrom(ch, bufferToFill.startSample, browserMixBuffer, ch, 0, numSamples);
    }

    if (state == Dashboard) {
        if (dashboardMixBuffer.getNumSamples() < numSamples) {
             dashboardMixBuffer.setSize(2, numSamples);
        }
        dashboardMixBuffer.clear(0, numSamples);

        juce::AudioSourceChannelInfo dashInfo(&dashboardMixBuffer, 0, numSamples);
        dashboardTransport.getNextAudioBlock(dashInfo);

        int dashCh = std::min(numChannels, dashboardMixBuffer.getNumChannels());
        for (int ch = 0; ch < dashCh; ++ch) {
            bufferToFill.buffer->addFrom(ch, bufferToFill.startSample, dashboardMixBuffer, ch, 0, numSamples);
        }
    }
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);
    engine.processPhysicalMidi(message);
}

bool MainComponent::keyPressed(const juce::KeyPress& key, Component* originatingComponent)
{
    juce::ignoreUnused(key, originatingComponent);
    if (state != Editor)
        return false;

    const auto runOverrideAction = [this](const std::string& id) -> bool
    {
        auto& appCmd = commandManager.getApplicationCommandManager();
        if (id == "play_stop") { appCmd.invokeDirectly(Orion::CommandManager::PlayStop, true); return true; }
        if (id == "stop_transport") { appCmd.invokeDirectly(Orion::CommandManager::Stop, true); return true; }
        if (id == "record_transport") { appCmd.invokeDirectly(Orion::CommandManager::Record, true); return true; }
        if (id == "rewind_transport") { appCmd.invokeDirectly(Orion::CommandManager::Rewind, true); return true; }
        if (id == "fast_forward_transport") { appCmd.invokeDirectly(Orion::CommandManager::FastForward, true); return true; }
        if (id == "panic_transport") { appCmd.invokeDirectly(Orion::CommandManager::Panic, true); return true; }
        if (id == "show_command_palette") { appCmd.invokeDirectly(Orion::CommandManager::ShowCommandPalette, true); return true; }
        if (id == "open_shortcut_cheatsheet") { appCmd.invokeDirectly(Orion::CommandManager::OpenShortcutCheatsheet, true); return true; }
        if (id == "save_project") { appCmd.invokeDirectly(Orion::CommandManager::Save, true); return true; }
        if (id == "load_project") { appCmd.invokeDirectly(Orion::CommandManager::Load, true); return true; }
        if (id == "add_instrument_track") { appCmd.invokeDirectly(Orion::CommandManager::AddInstrumentTrack, true); return true; }
        if (id == "add_audio_track") { appCmd.invokeDirectly(Orion::CommandManager::AddAudioTrack, true); return true; }
        if (id == "add_bus_track") { appCmd.invokeDirectly(Orion::CommandManager::AddBusTrack, true); return true; }
        if (id == "export_audio") { appCmd.invokeDirectly(Orion::CommandManager::Export, true); return true; }
        if (id == "undo_action") { appCmd.invokeDirectly(Orion::CommandManager::Undo, true); return true; }
        if (id == "redo_action") { appCmd.invokeDirectly(Orion::CommandManager::Redo, true); return true; }
        if (id == "delete_selection") { appCmd.invokeDirectly(Orion::CommandManager::DeleteSelection, true); return true; }
        if (id == "toggle_loop") { appCmd.invokeDirectly(Orion::CommandManager::ToggleLoop, true); return true; }
        if (id == "toggle_click") { appCmd.invokeDirectly(Orion::CommandManager::ToggleClick, true); return true; }
        if (id == "tool_select") { appCmd.invokeDirectly(Orion::CommandManager::ToolSelect, true); return true; }
        if (id == "tool_draw") { appCmd.invokeDirectly(Orion::CommandManager::ToolDraw, true); return true; }
        if (id == "tool_erase") { appCmd.invokeDirectly(Orion::CommandManager::ToolErase, true); return true; }
        if (id == "tool_split") { appCmd.invokeDirectly(Orion::CommandManager::ToolSplit, true); return true; }
        if (id == "tool_glue") { appCmd.invokeDirectly(Orion::CommandManager::ToolGlue, true); return true; }
        if (id == "tool_zoom") { appCmd.invokeDirectly(Orion::CommandManager::ToolZoom, true); return true; }
        if (id == "tool_mute") { appCmd.invokeDirectly(Orion::CommandManager::ToolMute, true); return true; }
        if (id == "tool_listen") { appCmd.invokeDirectly(Orion::CommandManager::ToolListen, true); return true; }
        if (id == "show_settings") { appCmd.invokeDirectly(Orion::CommandManager::ShowSettings, true); return true; }
        if (id == "show_guided_start") { appCmd.invokeDirectly(Orion::CommandManager::ShowGuidedStartWizard, true); return true; }
        if (id == "toggle_panel_inspector") { menuItemSelected(3001, 0); return true; }
        if (id == "toggle_panel_mixer") { menuItemSelected(3003, 0); return true; }
        if (id == "toggle_panel_piano_roll") { menuItemSelected(3004, 0); return true; }
        if (id == "toggle_panel_browser") { menuItemSelected(3002, 0); return true; }
        if (id == "workspace_compose") { applyWorkspacePreset(Orion::WorkspacePreset::Compose); return true; }
        if (id == "workspace_arrange") { applyWorkspacePreset(Orion::WorkspacePreset::Arrange); return true; }
        if (id == "workspace_mix") { applyWorkspacePreset(Orion::WorkspacePreset::Mix); return true; }
        return false;
    };

    for (const auto& it : Orion::SettingsManager::get().shortcutOverrides)
    {
        if (matchesShortcutSpec(key, it.second))
        {
            if (runOverrideAction(it.first))
                return true;
        }
    }

    if (key.getModifiers().isAltDown())
    {
        if (key.getTextCharacter() == '1') { menuItemSelected(3001, 0); return true; }
        if (key.getTextCharacter() == '2') { menuItemSelected(3003, 0); return true; }
        if (key.getTextCharacter() == '3') { menuItemSelected(3004, 0); return true; }
        if (key.getTextCharacter() == '4') { menuItemSelected(3002, 0); return true; }
    }

    return false;
}

bool MainComponent::keyStateChanged(bool isKeyDown, Component* originatingComponent)
{
    juce::ignoreUnused(isKeyDown, originatingComponent);
    if (state == Editor) {
        qwertyMidi.process([this](const juce::MidiMessage& m){
            engine.processPhysicalMidi(m);
        });
    }
    return false;
}

void MainComponent::releaseResources()
{
    Orion::Logger::info("MainComponent: releaseResources");
    browser.releaseResources();
    dashboardTransport.releaseResources();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::setTheme(const juce::String& themeName)
{
    auto& s = Orion::SettingsManager::get();
    auto resolvedThemeId = Orion::UiThemeRegistry::resolveThemeId(themeName.toStdString());
    auto resolvedDisplayName = juce::String(Orion::UiThemeRegistry::getDisplayName(resolvedThemeId));
    if (s.customThemes.contains(themeName.toStdString()))
    {
        auto themeData = s.customThemes[themeName.toStdString()];
        OrionLookAndFeel::ThemePalette palette = lnf.getThemePalette("Deep Space"); // Use as base

        if (themeData.contains("accent")) palette.accent = juce::Colour::fromString(themeData["accent"].get<std::string>());
        if (themeData.contains("chassis")) palette.chassis = juce::Colour::fromString(themeData["chassis"].get<std::string>());
        if (themeData.contains("panel")) palette.panel = juce::Colour::fromString(themeData["panel"].get<std::string>());

        lnf.setCustomTheme(themeName, palette);
    }

    s.themeId = resolvedThemeId;
    s.theme = resolvedDisplayName.toStdString();

    lnf.setTheme(resolvedDisplayName);
    sendLookAndFeelChange();
    repaint();
}

void MainComponent::resized()
{
    updateTitleBarForState();

    auto area = getLocalBounds();


    customTitleBar.setBounds(area.removeFromTop(30));
    customTitleBar.toFront(false);

    if (state == Editor)
        statusBar.setBounds(area.removeFromBottom(28));
    else
        statusBar.setVisible(false);

    if (windowResizer)
    {
        bool isWindowFixed = false;
        if (auto* w = dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent())) {
            // Treat as fixed if in full-screen or maximised-to-work-area
            if (w->isFullScreen()) {
                isWindowFixed = true;
            }

            if (auto* ow = dynamic_cast<OrionWindow*>(w))
                isWindowFixed = isWindowFixed || ow->isWorkAreaMaximised();
        }

        if (isWindowFixed) {
            windowResizer->setVisible(false);
        } else {
            windowResizer->setVisible(true);
            windowResizer->setBounds(getLocalBounds());
            windowResizer->toFront(false);
        }
    }


    customTitleBar.toFront(false);
    if (resizerV) resizerV->toFront(false);
    if (resizerLeft) resizerLeft->toFront(false);
    if (resizerRight) resizerRight->toFront(false);

    if (state == Dashboard) {
        dashboard.setBounds(area);
        dashboard.setVisible(true);
        guidedStartWizard.setVisible(false);

        header.setVisible(false);
        statusBar.setVisible(false);
        timeline.setVisible(false);
        inspectorPanel->setVisible(false);
        browserPanel->setVisible(false);
        mixerPanel->setVisible(false);
        editorPanel->setVisible(false);
        extensionsPanel->setVisible(false);
        resizerV->setVisible(false);
        resizerLeft->setVisible(false);
        resizerRight->setVisible(false);

        if (inspectorWindow) inspectorWindow->setVisible(false);
        if (browserWindow) browserWindow->setVisible(false);
        if (mixerWindow) mixerWindow->setVisible(false);
        if (editorWindow) editorWindow->setVisible(false);
        if (extensionsWindow) extensionsWindow->setVisible(false);

        updateTitleBarViewButtons();
        return;
    }

    bool comingFromDashboard = dashboard.isVisible();
    dashboard.setVisible(false);

    if (comingFromDashboard) {
        dashboardTransport.stop();
        dashboard.stopPlayback();
    }

    header.setVisible(true);
    statusBar.setVisible(true);
    timeline.setVisible(true);

    if (comingFromDashboard) {
         if (inspectorWindow) inspectorWindow->setVisible(true); else inspectorPanel->setVisible(true);
         if (browserWindow) browserWindow->setVisible(true); else browserPanel->setVisible(true);
         if (editorWindow) editorWindow->setVisible(true); else editorPanel->setVisible(true);

         if (mixerWindow) mixerWindow->setVisible(false); else mixerPanel->setVisible(false);

         if (extensionsWindow) extensionsWindow->setVisible(false);
         else if (extensionsPanel->isVisible()) extensionsPanel->setVisible(true);
    }

    bool showBottom = (editorPanel->isVisible() || mixerPanel->isVisible() || extensionsPanel->isVisible());

    if (showBottom) {

        double min, max, pref;
        verticalLayout.getItemLayout(3, min, max, pref);

        // Ensure minimal reasonable height for bottom panels (Mixer, Editor, etc)
        // Set minimal height to 350px, allow growing to fill available space
        if (max == 0.0 && pref == 0.0) {
            verticalLayout.setItemLayout(3, 350, -1.0, -0.5);
        } else if (min < 350) {
             // Correct minimal height if it was too small from saved state
             verticalLayout.setItemLayout(3, 350, max, pref);
        }


        verticalLayout.setItemLayout(2, 5, 5, 5);
        resizerV->setVisible(true);
    } else {
        verticalLayout.setItemLayout(3, 0, 0, 0);


        verticalLayout.setItemLayout(2, 0, 0, 0);
        resizerV->setVisible(false);
    }

    if (inspectorPanel->isVisible()) {
        double min = 0.0, max = 0.0, pref = 0.0;
        horizontalLayout.getItemLayout(0, min, max, pref);
        if (max == 0.0 && pref == 0.0) {
            horizontalLayout.setItemLayout(0, 240, 400, 260); // Constrain inspector width better
        }
        resizerLeft->setVisible(true);
    } else {
        horizontalLayout.setItemLayout(0, 0, 0, 0);
        resizerLeft->setVisible(false);
    }

    if (browserPanel->isVisible()) {
        double min = 0.0, max = 0.0, pref = 0.0;
        horizontalLayout.getItemLayout(4, min, max, pref);
        if (max == 0.0 && pref == 0.0) {
            horizontalLayout.setItemLayout(4, 220, 500, 280); // Better browser defaults
        }
        resizerRight->setVisible(true);
    } else {
        horizontalLayout.setItemLayout(4, 0, 0, 0);
        resizerRight->setVisible(false);
    }


    juce::Component* vComps[] = { &header, nullptr, resizerV.get(), nullptr };
    verticalLayout.layOutComponents(vComps, 4, area.getX(), area.getY(), area.getWidth(), area.getHeight(), true, true);

    int headerBottom = header.getBottom();
    int resizerY = resizerV->getY();
    int resizerH = resizerV->getHeight();
    if (!resizerV->isVisible()) {
        resizerH = 0;
        resizerY = area.getBottom();
    }

    if (resizerY > area.getBottom()) resizerY = area.getBottom();



    if (showBottom && (resizerY - headerBottom < 20))
    {

        verticalLayout.setItemLayout(1, 200, -1.0, -0.5);
        verticalLayout.setItemLayout(3, 350, -1.0, -0.5);


        verticalLayout.layOutComponents(vComps, 4, area.getX(), area.getY(), area.getWidth(), area.getHeight(), true, true);
        resizerY = resizerV->getY();
    }


    juce::Rectangle<int> middleArea(area.getX(), headerBottom, area.getWidth(), std::max(0, resizerY - headerBottom));
    juce::Rectangle<int> bottomArea(area.getX(), resizerY + resizerH, area.getWidth(), std::max(0, area.getBottom() - (resizerY + resizerH)));

    if (!showBottom) {
        middleArea.setBottom(area.getBottom());
        bottomArea.setHeight(0);
    }

    juce::Component* hComps[] = {
        inspectorPanel->isVisible() ? inspectorPanel.get() : nullptr,
        resizerLeft.get(),
        &timeline,
        resizerRight.get(),
        browserPanel->isVisible() ? browserPanel.get() : nullptr
    };
    horizontalLayout.layOutComponents(hComps, 5, middleArea.getX(), middleArea.getY(), middleArea.getWidth(), middleArea.getHeight(), false, true);

    if (showBottom && bottomArea.getHeight() > 0)
    {
        std::vector<juce::Component*> bottomComps;
        if (mixerPanel->isVisible()) bottomComps.push_back(mixerPanel.get());
        if (editorPanel->isVisible()) bottomComps.push_back(editorPanel.get());
        if (extensionsPanel->isVisible()) bottomComps.push_back(extensionsPanel.get());

        if (!bottomComps.empty()) {
            int w = bottomArea.getWidth() / (int)bottomComps.size();
            for (size_t i=0; i<bottomComps.size(); ++i) {
                if (i == bottomComps.size() - 1) bottomComps[i]->setBounds(bottomArea);
                else bottomComps[i]->setBounds(bottomArea.removeFromLeft(w));
            }
        }
    }

    updateTitleBarViewButtons();

    updateGuidedStartVisibility();
    if (guidedStartWizard.isVisible())
    {
        guidedStartWizard.setBounds(getLocalBounds());
        guidedStartWizard.toFront(false);
    }
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit", "Tools", "View", "Help" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& menuName)
{
    juce::ignoreUnused(menuIndex);
    juce::PopupMenu menu;
    auto& cmdMgr = commandManager.getApplicationCommandManager();

    if (menuName == "File")
    {
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::AddInstrumentTrack, "New Project");
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::Load);
        if (state == Editor) menu.addCommandItem(&cmdMgr, Orion::CommandManager::Save);
        menu.addSeparator();
        if (state == Editor) {
            menu.addItem(998, "Close Project");
            menu.addCommandItem(&cmdMgr, Orion::CommandManager::Export);
            menu.addSeparator();
        }
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::ShowSettings);
        menu.addSeparator();
        menu.addItem(999, "Exit", true, false);
    }
    else if (menuName == "Edit")
    {
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::Undo);
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::Redo);
        menu.addSeparator();
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::ShowCommandPalette);
        menu.addSeparator();
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::DeleteSelection);
    }
    else if (menuName == "Tools")
    {
        menu.addSectionHeader("Track Operations");
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::AddAudioTrack);
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::AddInstrumentTrack);
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::AddBusTrack);
    }
    else if (menuName == "View")
    {
        const bool panelsEnabled = (state != Dashboard);
        menu.addItem(3001, "Inspector", panelsEnabled, inspectorWindow ? inspectorWindow->isVisible() : inspectorPanel->isVisible());
        menu.addItem(3002, "Browser", panelsEnabled, browserWindow ? browserWindow->isVisible() : browserPanel->isVisible());
        menu.addItem(3003, "Mixer", panelsEnabled, mixerWindow ? mixerWindow->isVisible() : mixerPanel->isVisible());
        menu.addItem(3004, "Piano Roll", panelsEnabled, editorWindow ? editorWindow->isVisible() : editorPanel->isVisible());

        if (Orion::SettingsManager::get().developerMode) {
             menu.addItem(3005, "Extensions", panelsEnabled, extensionsWindow ? extensionsWindow->isVisible() : extensionsPanel->isVisible());
        }

        menu.addSeparator();
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::ShowGuidedStartWizard);
        menu.addSeparator();
        juce::PopupMenu wsMenu;
        wsMenu.addItem(3101, "Compose", panelsEnabled, currentWorkspacePreset == Orion::WorkspacePreset::Compose);
        wsMenu.addItem(3102, "Arrange", panelsEnabled, currentWorkspacePreset == Orion::WorkspacePreset::Arrange);
        wsMenu.addItem(3103, "Mix", panelsEnabled, currentWorkspacePreset == Orion::WorkspacePreset::Mix);
        menu.addSubMenu("Workspace Preset", wsMenu);
        menu.addSeparator();

        juce::PopupMenu themeMenu;
        auto themes = lnf.getAvailableThemes();
        for (int i = 0; i < themes.size(); ++i)
        {
            themeMenu.addItem(4001 + i, themes[i], true, lnf.getCurrentTheme() == themes[i]);
        }
        menu.addSubMenu("Theme", themeMenu);
    }
    else if (menuName == "Help")
    {
        menu.addCommandItem(&cmdMgr, Orion::CommandManager::OpenShortcutCheatsheet);
        menu.addItem(2002, "Documentation");
        menu.addItem(2001, "About Orion");
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);

    if (menuItemID == 998)
    {
        promptToSaveAndClose();
    }
    else if (menuItemID == 999)
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    else if (menuItemID == 3001)
    {
        if (inspectorWindow) inspectorWindow->setVisible(!inspectorWindow->isVisible());
        else { inspectorPanel->setVisible(!inspectorPanel->isVisible()); resized(); }
    }
    else if (menuItemID == 3002)
    {
        if (browserWindow) browserWindow->setVisible(!browserWindow->isVisible());
        else { browserPanel->setVisible(!browserPanel->isVisible()); resized(); }
    }
    else if (menuItemID == 3003)
    {
        if (mixerWindow) mixerWindow->setVisible(!mixerWindow->isVisible());
        else { mixerPanel->setVisible(!mixerPanel->isVisible()); resized(); }
    }
    else if (menuItemID == 3004)
    {
        if (editorWindow) editorWindow->setVisible(!editorWindow->isVisible());
        else { editorPanel->setVisible(!editorPanel->isVisible()); resized(); }
    }
    else if (menuItemID == 3005)
    {
        if (extensionsWindow) extensionsWindow->setVisible(!extensionsWindow->isVisible());
        else { extensionsPanel->setVisible(!extensionsPanel->isVisible()); resized(); }
    }
    else if (menuItemID == 3101)
    {
        applyWorkspacePreset(Orion::WorkspacePreset::Compose);
    }
    else if (menuItemID == 3102)
    {
        applyWorkspacePreset(Orion::WorkspacePreset::Arrange);
    }
    else if (menuItemID == 3103)
    {
        applyWorkspacePreset(Orion::WorkspacePreset::Mix);
    }
    else if (menuItemID >= 4001 && menuItemID < 4100)
    {
        auto themes = lnf.getAvailableThemes();
        int index = menuItemID - 4001;
        if (index >= 0 && index < themes.size())
        {
            auto s = Orion::SettingsManager::get();
            s.theme = themes[index].toStdString();
            s.themeId = Orion::UiThemeRegistry::resolveThemeId(s.theme);
            Orion::SettingsManager::set(s);
            setTheme(themes[index]);
        }
    }
    else if (menuItemID == 2001)
    {
        showAbout();
    }
    else if (menuItemID == 2002)
    {
        showDocumentation();
    }
}

void MainComponent::closeProject()
{
    auto settings = Orion::SettingsManager::get();
    settings.projectLayoutState = getLayoutState();
    Orion::SettingsManager::set(settings);

    engine.cancelPreviewRender();
    if (previewThread.joinable()) previewThread.join();

    engine.stop();
    engine.panic();
    engine.clearTracks();
    commandHistory.clear();
    currentProjectFile = "";
    firstPlaybackLogged = false;
    forceShowGuidedStart = false;

    state = Dashboard;
    dashboard.refreshProjects();
    resized();
}

void MainComponent::saveProject(std::function<void(bool)> onComplete)
{
    std::string layoutState = getLayoutState();
    auto settings = Orion::SettingsManager::get();
    settings.projectLayoutState = layoutState;
    Orion::SettingsManager::set(settings);
    if (!currentProjectFile.empty())
    {
        bool success = Orion::ProjectSerializer::save(engine, currentProjectFile, layoutState);
        if (success)
        {
            commandHistory.setSavePoint();
            startAutoSave();

            engine.cancelPreviewRender();
            if (previewThread.joinable()) previewThread.join();

            previewThread = std::thread([this, path = currentProjectFile]() {
                std::string previewPath = std::filesystem::path(path).parent_path().string() + "/preview.wav";

                Orion::AudioEngine backgroundEngine;
                if (Orion::ProjectSerializer::load(backgroundEngine, path)) {
                    backgroundEngine.renderPreview(previewPath);
                }

                juce::Component::SafePointer<MainComponent> sp(this);
                juce::MessageManager::callAsync([sp](){
                    if (sp != nullptr && sp->state == Dashboard) {
                        sp->dashboard.refreshProjects();
                    }
                });
            });
        }
        else
        {
             juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Failed to save project.");
        }

        if (onComplete) onComplete(success);
    }
    else
    {
        auto file = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("Project.orion");

        auto chooser = std::make_shared<juce::FileChooser>("Save Project", file, "*.orion");
        juce::Component::SafePointer<MainComponent> sp(this);
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [sp, chooser, onComplete, layoutState](const juce::FileChooser& fc)
            {
                if (sp && fc.getURLResults().size() > 0)
                {
                    std::string path = fc.getResult().getFullPathName().toStdString();
                    if (Orion::ProjectSerializer::save(sp->engine, path, layoutState))
                    {
                        sp->currentProjectFile = path;
                        sp->commandHistory.setSavePoint();
                        sp->startAutoSave();

                        sp->engine.cancelPreviewRender();
                        if (sp->previewThread.joinable()) sp->previewThread.join();

                        sp->previewThread = std::thread([self = (MainComponent*)sp, path]() {
                            Orion::AudioEngine backgroundEngine;
                            if (Orion::ProjectSerializer::load(backgroundEngine, path)) {
                                backgroundEngine.renderPreview(std::filesystem::path(path).parent_path().string() + "/preview.wav");
                            }

                            juce::Component::SafePointer<MainComponent> spChild(self);
                            juce::MessageManager::callAsync([spChild](){
                                if (spChild != nullptr && spChild->state == Dashboard) {
                                    spChild->dashboard.refreshProjects();
                                }
                            });
                        });

                        if (onComplete) onComplete(true);
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Failed to save project.");
                        if (onComplete) onComplete(false);
                    }
                }
                else
                {
                    if (onComplete) onComplete(false);
                }
            });
    }
}

void MainComponent::detachPanel(DockablePanel* panel, juce::Component* content, std::unique_ptr<DetachedWindow>* window, const juce::String& name)
{
    panel->setVisible(false);

    *window = std::make_unique<DetachedWindow>(name, content, [this, panel, content, window] {
        attachPanel(panel, content, window);
    });

    resized();
}

void MainComponent::attachPanel(DockablePanel* panel, juce::Component* content, std::unique_ptr<DetachedWindow>* window)
{
    juce::MessageManager::callAsync([this, panel, content, window] {
        if (!window || !*window) return;
        panel->addAndMakeVisible(content);
        panel->setVisible(true);
        window->reset();
        resized();
    });
}

std::string MainComponent::getLayoutState() const {
    json j;
    j["layoutSchemaVersion"] = 2;
    j["inspectorVisible"] = inspectorPanel->isVisible();
    j["browserVisible"] = browserPanel->isVisible();
    j["mixerVisible"] = mixerPanel->isVisible();
    j["editorVisible"] = editorPanel->isVisible();
    j["extensionsVisible"] = extensionsPanel->isVisible();

    json jVert = json::array();
    for (int i=0; i<4; ++i) {
        double min, max, pref;
        if (const_cast<juce::StretchableLayoutManager&>(verticalLayout).getItemLayout(i, min, max, pref)) {
             jVert.push_back({min, max, pref});
        } else {
             jVert.push_back({0,0,0});
        }
    }
    j["verticalLayout"] = jVert;

    json jHorz = json::array();
    for (int i=0; i<5; ++i) {
        double min, max, pref;
        if (const_cast<juce::StretchableLayoutManager&>(horizontalLayout).getItemLayout(i, min, max, pref)) {
             jHorz.push_back({min, max, pref});
        } else {
             jHorz.push_back({0,0,0});
        }
    }
    j["horizontalLayout"] = jHorz;


    auto currentClip = editor.getCurrentClip();
    auto currentTrack = editor.getCurrentTrack();

    if (currentClip && currentTrack) {
        int trackIdx = -1;
        int clipIdx = -1;
        auto tracks = engine.getTracks();
        for(size_t i=0; i<tracks.size(); ++i) {
            if (tracks[i] == currentTrack) {
                trackIdx = (int)i;
                if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(tracks[i])) {
                    if (auto clips = inst->getMidiClips()) {
                        for(size_t k=0; k<clips->size(); ++k) {
                            if ((*clips)[k] == currentClip) {
                                clipIdx = (int)k;
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
        if (trackIdx != -1 && clipIdx != -1) {
            j["editorState"] = { {"trackIndex", trackIdx}, {"clipIndex", clipIdx} };
        }
    }

    if (getPeer()) {
        auto bounds = getPeer()->getBounds();
        j["mainWindowBounds"] = { bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight() };
    }

    j["timeline"] = {
        {"zoomX", timeline.getPixelsPerSecond()},
        {"zoomY", timeline.getTrackHeight()},
        {"scrollX", timeline.getScrollX()},
        {"scrollY", timeline.getScrollY()}
    };

    j["mixer"] = {
        {"scrollX", mixer.getScrollX()}
    };

    j["browserState"] = browser.getState();
    j["workspacePreset"] = (int)currentWorkspacePreset;
    j["guidedStart"] = {
        {"dismissed", guidedStartState.dismissed},
        {"lastTrackType", guidedStartState.lastTrackType == Orion::GuidedStartTrackType::Audio ? "audio" : "instrument"},
        {"lastBpm", guidedStartState.lastBpm},
        {"lastLoopBars", guidedStartState.lastLoopBars}
    };

    auto saveDetached = [&](const std::string& key, DetachedWindow* window) {
        if (window && window->isVisible()) {
            auto bounds = window->getBounds();
            j["detachedWindows"][key] = {
                {"detached", true},
                {"bounds", {bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()}}
            };
        }
    };

    saveDetached("inspector", inspectorWindow.get());
    saveDetached("browser", browserWindow.get());
    saveDetached("mixer", mixerWindow.get());
    saveDetached("editor", editorWindow.get());
    saveDetached("extensions", extensionsWindow.get());

    return j.dump();
}

void MainComponent::applyLayoutState(const std::string& stateStr) {
    if (stateStr.empty()) return;
    try {
        json j = json::parse(stateStr);
        forceShowGuidedStart = false;
        const int layoutSchemaVersion = j.value("layoutSchemaVersion", 1);
        auto settingsCopy = Orion::SettingsManager::get();
        if (settingsCopy.panelLayoutSchemaVersion != layoutSchemaVersion) {
            settingsCopy.panelLayoutSchemaVersion = layoutSchemaVersion;
            Orion::SettingsManager::set(settingsCopy);
        }

        if (j.contains("verticalLayout")) {
            if (j["verticalLayout"].size() == 5) {

                auto getItem = [&](int idx) { return j["verticalLayout"][idx]; };
                auto apply = [&](int idx, const json& item) {
                    verticalLayout.setItemLayout(idx, item[0], item[1], item[2]);
                };

                apply(0, getItem(0));
                apply(1, getItem(2));
                apply(2, getItem(3));
                apply(3, getItem(4));
            } else {
                int i=0;
                for(const auto& item : j["verticalLayout"]) {
                    if (i >= 4) break;
                    verticalLayout.setItemLayout(i++, item[0], item[1], item[2]);
                }
            }
        }
        if (j.contains("horizontalLayout")) {
            int i=0;
            for(const auto& item : j["horizontalLayout"]) {
                horizontalLayout.setItemLayout(i++, item[0], item[1], item[2]);
            }
        }

        inspectorPanel->setVisible(j.value("inspectorVisible", false));
        browserPanel->setVisible(j.value("browserVisible", true));

        mixerPanel->setVisible(j.value("mixerVisible", true));
        // Strict adherence to saved state or default to false
        editorPanel->setVisible(j.value("editorVisible", false));
        extensionsPanel->setVisible(j.value("extensionsVisible", Orion::SettingsManager::get().developerMode));

        if (j.contains("detachedWindows")) {
            auto applyDetached = [&](const std::string& key, DockablePanel* panel, juce::Component* content, std::unique_ptr<DetachedWindow>* window, const juce::String& name) {
                if (j["detachedWindows"].contains(key)) {
                    auto wInfo = j["detachedWindows"][key];
                    if (wInfo.value("detached", false)) {
                        detachPanel(panel, content, window, name);
                        if (wInfo.contains("bounds") && *window) {
                            auto b = wInfo["bounds"];
                            if (b.size() == 4) {
                                (*window)->setBounds(b[0], b[1], b[2], b[3]);
                            }
                        }
                    }
                }
            };

            applyDetached("inspector", inspectorPanel.get(), &inspector, &inspectorWindow, "Inspector");
            applyDetached("browser", browserPanel.get(), &browser, &browserWindow, "Browser");
            applyDetached("mixer", mixerPanel.get(), &mixer, &mixerWindow, "Mixer");
            applyDetached("editor", editorPanel.get(), &editor, &editorWindow, "Piano Roll");
            applyDetached("extensions", extensionsPanel.get(), &extensionView, &extensionsWindow, "Extensions");
        }

        if (j.contains("browserState")) {
            browser.restoreState(j["browserState"].get<std::string>());
        }

        if (j.contains("workspacePreset")) {
            int ws = j["workspacePreset"];
            if (ws == 1) currentWorkspacePreset = Orion::WorkspacePreset::Arrange;
            else if (ws == 2) currentWorkspacePreset = Orion::WorkspacePreset::Mix;
            else currentWorkspacePreset = Orion::WorkspacePreset::Compose;
        }

        if (j.contains("guidedStart")) {
            auto gs = j["guidedStart"];
            guidedStartState.dismissed = gs.value("dismissed", false);
            const auto lastTrackType = gs.value("lastTrackType", std::string("instrument"));
            guidedStartState.lastTrackType = (lastTrackType == "audio")
                ? Orion::GuidedStartTrackType::Audio
                : Orion::GuidedStartTrackType::Instrument;
            guidedStartState.lastBpm = gs.value("lastBpm", 120.0f);
            guidedStartState.lastLoopBars = gs.value("lastLoopBars", 8);
        } else if (j.contains("quickStart")) {
            // Backward compatibility with legacy quick-start state.
            auto qs = j["quickStart"];
            guidedStartState.dismissed = qs.value("dismissed", false);
            guidedStartState.lastTrackType = Orion::GuidedStartTrackType::Instrument;
            guidedStartState.lastBpm = 120.0f;
            guidedStartState.lastLoopBars = 8;
        }
        guidedStartWizard.setState(guidedStartState);

        verticalLayout.setItemLayout(2, 5, 5, 5);
        horizontalLayout.setItemLayout(1, 5, 5, 5);
        horizontalLayout.setItemLayout(3, 5, 5, 5);


        if (j.contains("editorState")) {
            int trackIdx = j["editorState"].value("trackIndex", -1);
            int clipIdx = j["editorState"].value("clipIndex", -1);

            if (trackIdx != -1 && clipIdx != -1) {
                auto tracks = engine.getTracks();
                if (trackIdx < (int)tracks.size()) {
                    if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(tracks[trackIdx])) {
                        if (auto clips = inst->getMidiClips()) {
                            if (clipIdx < (int)clips->size()) {
                                editor.showPianoRoll((*clips)[clipIdx], inst);
                            }
                        }
                    }
                }
            }
        }

        if (j.contains("timeline")) {
            auto t = j["timeline"];
            if (t.contains("zoomX")) timeline.setPixelsPerSecond(t["zoomX"]);
            if (t.contains("zoomY")) timeline.setTrackHeight(t["zoomY"]);
            if (t.contains("scrollX") && t.contains("scrollY")) {
                timeline.setViewPosition(t["scrollX"], t["scrollY"]);
            }
        }

        if (j.contains("mixer")) {
            auto m = j["mixer"];
            if (m.contains("scrollX")) mixer.setViewPosition(m["scrollX"]);
        }

        // We no longer restore window bounds here to prevent automatic maximization on project load
        updateGuidedStartVisibility();

    } catch(...) {}
}

void MainComponent::updateTitleBarForState()
{
    customTitleBar.setShowMenuBar(true);
    customTitleBar.setShowViewButtons(false);

    juce::File f(currentProjectFile);
    juce::String name = f.existsAsFile() ? f.getFileNameWithoutExtension() : "";
    if (commandHistory.isDirty()) name += "*";
    customTitleBar.setProjectName(name);
}

void MainComponent::updateTitleBarViewButtons()
{
    const bool showEditor = (inspectorWindow ? inspectorWindow->isVisible() : inspectorPanel->isVisible());
    const bool showMixer = (mixerWindow ? mixerWindow->isVisible() : mixerPanel->isVisible());
    const bool showPiano = (editorWindow ? editorWindow->isVisible() : editorPanel->isVisible());
    const bool showBrowser = (browserWindow ? browserWindow->isVisible() : browserPanel->isVisible());
    statusBar.setViewButtonStates(showEditor, showMixer, showPiano, showBrowser);
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
    return &commandManager;
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.add(Orion::CommandManager::Load);
    commands.add(Orion::CommandManager::Save);
    commands.add(Orion::CommandManager::Export);
    commands.add(Orion::CommandManager::Undo);
    commands.add(Orion::CommandManager::Redo);

    commands.add(Orion::CommandManager::DeleteSelection);
    commands.add(Orion::CommandManager::ToolSelect);
    commands.add(Orion::CommandManager::ToolDraw);
    commands.add(Orion::CommandManager::ToolErase);
    commands.add(Orion::CommandManager::ToolSplit);
    commands.add(Orion::CommandManager::ToolListen);

    commands.add(Orion::CommandManager::AddAudioTrack);
    commands.add(Orion::CommandManager::AddInstrumentTrack);
    commands.add(Orion::CommandManager::AddBusTrack);

    commands.add(Orion::CommandManager::ShowSettings);
    commands.add(Orion::CommandManager::ShowCommandPalette);
    commands.add(Orion::CommandManager::OpenShortcutCheatsheet);
    commands.add(Orion::CommandManager::ShowGuidedStartWizard);
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    if (commandID == Orion::CommandManager::ShowSettings)
    {
        result.setInfo("Settings", "Open Settings", "File", 0);
        result.addDefaultKeypress('P', juce::ModifierKeys::commandModifier);
        return;
    }

    commandManager.getCommandInfo(commandID, result);

    // Grey out project-specific commands when on the home screen
    if (state == Dashboard)
    {
        switch (commandID)
        {
            case Orion::CommandManager::Save:
            case Orion::CommandManager::Export:
            case Orion::CommandManager::Undo:
            case Orion::CommandManager::Redo:
            case Orion::CommandManager::AddAudioTrack:
            case Orion::CommandManager::AddBusTrack:
            case Orion::CommandManager::DeleteSelection:
            case Orion::CommandManager::ShowCommandPalette:
            case Orion::CommandManager::ShowGuidedStartWizard:
            case Orion::CommandManager::PlayStop:
            case Orion::CommandManager::Stop:
            case Orion::CommandManager::Record:
            case Orion::CommandManager::ToggleLoop:
            case Orion::CommandManager::ToggleClick:
            case Orion::CommandManager::ReturnToZero:
            case Orion::CommandManager::Rewind:
            case Orion::CommandManager::FastForward:
            case Orion::CommandManager::Panic:
            case Orion::CommandManager::ToolSelect:
            case Orion::CommandManager::ToolDraw:
            case Orion::CommandManager::ToolErase:
            case Orion::CommandManager::ToolSplit:
            case Orion::CommandManager::ToolListen:
                result.setActive(false);
                break;
            case Orion::CommandManager::Load:
            case Orion::CommandManager::AddInstrumentTrack: // Used for "New Project"
            case Orion::CommandManager::ShowSettings:
                result.setActive(true);
                break;
            default:
                break;
        }
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case Orion::CommandManager::Save:
        {
            saveProject(nullptr);
            return true;
        }
        case Orion::CommandManager::Load:
        {
            auto chooser = std::make_shared<juce::FileChooser>("Open Project", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.orion");
            juce::Component::SafePointer<MainComponent> spMain(this);
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [spMain, chooser](const juce::FileChooser& fc)
                {
                    if (spMain && fc.getURLResults().size() > 0)
                    {
                        std::string path = fc.getResult().getFullPathName().toStdString();
                        if (!spMain->loadingWindow) spMain->loadingWindow = std::make_unique<LoadingWindow>();
                        spMain->loadingWindow->show();

                        juce::Component::SafePointer<MainComponent> sp(spMain);
                        juce::MessageManager::callAsync([sp, path](){
                            if (!sp) return;

                            sp->engine.cancelPreviewRender();
                            if (sp->previewThread.joinable()) sp->previewThread.join();

                            std::string extraData;
                            if (Orion::ProjectSerializer::load(sp->engine, path, &extraData))
                            {
                                sp->currentProjectFile = path;
                                sp->state = Editor;
                                sp->firstPlaybackLogged = false;
                                sp->commandHistory.clear();
                                sp->commandHistory.setSavePoint();
                                sp->applyLayoutState(extraData);
                                sp->resized();
                                sp->timeline.refresh();
                                sp->mixer.refresh();
                            }
                            else
                            {
                                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Failed to load project.");
                            }
                            if (sp->loadingWindow) sp->loadingWindow->hide();
                        });
                    }
                });
            return true;
        }
        case Orion::CommandManager::Export:
        {
            engine.cancelPreviewRender();
            if (previewThread.joinable()) previewThread.join();

            if (engine.getTracks().empty())
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                       "Export blocked",
                                                       "Add at least one track before exporting.");
                return true;
            }

            auto* dialog = new juce::AlertWindow("Export Project", {}, juce::MessageBoxIconType::NoIcon);
            dialog->setLookAndFeel(&lnf);

            auto* exportView = new ExportSettingsComponent(engine);
            exportView->setSize(500, 480);
            dialog->addCustomComponent(exportView);

            auto* rangeHint = new juce::Label({}, "");
            rangeHint->setJustificationType(juce::Justification::centredLeft);
            rangeHint->setColour(juce::Label::textColourId, juce::Colours::grey);
            rangeHint->setFont(juce::FontOptions(12.0f, juce::Font::italic));
            dialog->addCustomComponent(rangeHint);
            exportView->setRangeHintLabel(rangeHint);

            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            dialog->addButton("Render...", 1, juce::KeyPress(juce::KeyPress::returnKey));

            dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog, exportView](int result)
            {
                auto options = exportView->getOptions();
                dialog->setLookAndFeel(nullptr);
                delete dialog;

                if (result != 1) return;

                juce::File baseDir = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
                const bool wantsMp3 = options.format == Orion::ExportFormat::Mp3;
                juce::String wildcard = wantsMp3 ? "*.mp3" : "*.wav";
                juce::String defaultName = options.fileName + (wantsMp3 ? ".mp3" : ".wav");

                auto chooser = std::make_shared<juce::FileChooser>("Export Audio", baseDir.getChildFile(defaultName), wildcard);
                juce::Component::SafePointer<MainComponent> sp(this);
                chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [sp, chooser, options](const juce::FileChooser& fc)
                    {
                        if (!sp || fc.getURLResults().size() == 0) return;
                        const bool wantsMp3 = options.format == Orion::ExportFormat::Mp3;

                        if (options.range == Orion::ExportRange::LoopRegion && sp->engine.getLoopEnd() <= sp->engine.getLoopStart()) {
                            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                                   "Export blocked",
                                                                   "Loop range is empty. Set loop points or choose a different range.");
                            return;
                        }

                        if (auto* master = sp->engine.getMasterNode()) {
                            float maxLevel = std::max(master->getLevelL(), master->getLevelR());
                            if (juce::Decibels::gainToDecibels(maxLevel, -100.0f) > 0.1f) {
                                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                                       "Possible clipping",
                                                                       "Master output appears to peak above 0 dB.");
                            }
                        }

                        juce::File file = fc.getResult();
                        juce::String expectedExt = wantsMp3 ? ".mp3" : ".wav";
                        if (!file.getFileExtension().equalsIgnoreCase(expectedExt))
                            file = file.withFileExtension(expectedExt);

                        Orion::ExportConfig cfg;
                        cfg.outputFile = file.getFullPathName().toStdString();
                        cfg.format = options.format;
                        cfg.range = options.range;
                        cfg.includeTail = options.includeTail;
                        cfg.targetSampleRate = options.sampleRate;
                        cfg.bitDepth = options.bitDepth;
                        cfg.mp3Bitrate = options.mp3Bitrate;

                        bool ok = sp->engine.renderProject(cfg);

                        if (ok)
                        {
                            juce::String message = "Export complete:\n" + file.getFullPathName();
                            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Success", message);
                            Orion::UxTelemetry::logEvent(Orion::UxEventType::FirstExport, {{"path", file.getFullPathName().toStdString()}});
                        }
                        else
                        {
                            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                                    "Export failed",
                                                                    "Could not export audio.\n" + juce::String(sp->engine.getLastExportError()));
                        }
                    });
            }));
            return true;
        }
        case Orion::CommandManager::Undo:
            if (commandHistory.canUndo())
                commandHistory.undo();
            return true;
        case Orion::CommandManager::Redo:
            if (commandHistory.canRedo())
                commandHistory.redo();
            return true;

        case Orion::CommandManager::DeleteSelection:
            timeline.deleteSelection();
            return true;

        case Orion::CommandManager::ToolSelect:
            timeline.setTool(TimelineTool::Select);
            return true;
        case Orion::CommandManager::ToolDraw:
            timeline.setTool(TimelineTool::Draw);
            return true;
        case Orion::CommandManager::ToolErase:
            timeline.setTool(TimelineTool::Erase);
            return true;
        case Orion::CommandManager::ToolSplit:
            timeline.setTool(TimelineTool::Split);
            return true;
        case Orion::CommandManager::ToolGlue:
            timeline.setTool(TimelineTool::Glue);
            return true;
        case Orion::CommandManager::ToolZoom:
            timeline.setTool(TimelineTool::Zoom);
            return true;
        case Orion::CommandManager::ToolMute:
            timeline.setTool(TimelineTool::Mute);
            return true;
        case Orion::CommandManager::ToolListen:
            timeline.setTool(TimelineTool::Listen);
            return true;

        case Orion::CommandManager::AddAudioTrack:
        {
            auto t = engine.createTrack();
            engine.removeTrack(t);
            commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
                t,
                [this](auto track) { engine.addTrack(track); timeline.refresh(); mixer.refresh(); },
                [this](auto track) { engine.removeTrack(track); timeline.refresh(); mixer.refresh(); }
            ));
            return true;
        }
        case Orion::CommandManager::AddInstrumentTrack:
        {
            if (state == Dashboard) {
                dashboard.createNewProject();
                return true;
            }
            auto t = engine.createInstrumentTrack();
            engine.removeTrack(t);
            commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
                t,
                [this](auto track) { engine.addTrack(track); timeline.refresh(); mixer.refresh(); },
                [this](auto track) { engine.removeTrack(track); timeline.refresh(); mixer.refresh(); }
            ));
            return true;
        }
        case Orion::CommandManager::AddBusTrack:
        {
            auto t = engine.createBusTrack();
            engine.removeTrack(t);
            commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
                t,
                [this](auto track) { engine.addTrack(track); timeline.refresh(); mixer.refresh(); },
                [this](auto track) { engine.removeTrack(track); timeline.refresh(); mixer.refresh(); }
            ));
            return true;
        }

        case Orion::CommandManager::ShowSettings:
        {
            showSettings();
            return true;
        }
        case Orion::CommandManager::ShowCommandPalette:
        {
            showCommandPalette();
            return true;
        }
        case Orion::CommandManager::OpenShortcutCheatsheet:
        {
            showShortcutCheatsheet();
            return true;
        }
        case Orion::CommandManager::ShowGuidedStartWizard:
        {
            showGuidedStartWizard(true);
            return true;
        }

        default:
            return false;
    }
}

void MainComponent::showCommandPalette()
{
    if (state != Editor)
        return;

    if (commandPaletteCallout != nullptr)
    {
        commandPaletteCallout->dismiss();
        commandPaletteCallout = nullptr;
    }

    auto palette = std::make_unique<CommandPaletteComponent>();
    palette->setSize(560, 340);

    std::vector<CommandPaletteComponent::Entry> entries;
    entries.push_back({"add_instrument", "Add Instrument Track", "Cmd/Ctrl+I", "track instrument add", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::AddInstrumentTrack, true);
    }});
    entries.push_back({"add_audio", "Add Audio Track", "Cmd/Ctrl+T", "track audio add", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::AddAudioTrack, true);
    }});
    entries.push_back({"create_clip", "Create Starter MIDI Clip", "", "midi clip create starter", [this] {
        timeline.createStarterMidiClip();
    }});
    entries.push_back({"guided_start", "Quick Start Setup", "Cmd/Ctrl+Shift+G", "onboarding start music quick", [this] {
        showGuidedStartWizard(true);
    }});
    entries.push_back({"toggle_loop", "Toggle Loop", "/", "transport loop", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::ToggleLoop, true);
    }});
    entries.push_back({"toggle_click", "Toggle Metronome", "C", "transport metronome click", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::ToggleClick, true);
    }});
    entries.push_back({"undo", "Undo", "Cmd/Ctrl+Z", "edit undo", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::Undo, true);
    }});
    entries.push_back({"redo", "Redo", "Cmd/Ctrl+Shift+Z", "edit redo", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::Redo, true);
    }});
    entries.push_back({"settings", "Open Settings", "Cmd/Ctrl+P", "preferences settings", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::ShowSettings, true);
    }});
    entries.push_back({"export", "Export Audio", "Cmd/Ctrl+E", "render export", [this] {
        commandManager.getApplicationCommandManager().invokeDirectly(Orion::CommandManager::Export, true);
    }});
    entries.push_back({"show_browser", "Toggle Browser Panel", "Alt+4", "view panel browser", [this] {
        menuItemSelected(3002, 0);
    }});
    entries.push_back({"show_mixer", "Toggle Mixer Panel", "Alt+2", "view panel mixer", [this] {
        menuItemSelected(3003, 0);
    }});
    entries.push_back({"show_piano", "Toggle Piano Roll Panel", "Alt+3", "view panel piano roll", [this] {
        menuItemSelected(3004, 0);
    }});
    entries.push_back({"show_inspector", "Toggle Inspector Panel", "Alt+1", "view panel inspector", [this] {
        menuItemSelected(3001, 0);
    }});
    entries.push_back({"ws_compose", "Workspace: Compose", "", "workspace compose", [this] {
        applyWorkspacePreset(Orion::WorkspacePreset::Compose);
    }});
    entries.push_back({"ws_arrange", "Workspace: Arrange", "", "workspace arrange", [this] {
        applyWorkspacePreset(Orion::WorkspacePreset::Arrange);
    }});
    entries.push_back({"ws_mix", "Workspace: Mix", "", "workspace mix", [this] {
        applyWorkspacePreset(Orion::WorkspacePreset::Mix);
    }});

    palette->setEntries(std::move(entries));
    palette->onClose = [this] {
        if (commandPaletteCallout != nullptr)
        {
            commandPaletteCallout->dismiss();
            commandPaletteCallout = nullptr;
        }
    };

    auto target = header.getScreenBounds().withTrimmedLeft(header.getWidth() / 3).withHeight(16);
    auto& box = juce::CallOutBox::launchAsynchronously(std::move(palette), target, nullptr);
    box.setArrowSize(0.0f);
    commandPaletteCallout = &box;
}

void MainComponent::showShortcutCheatsheet()
{
    const juce::String text =
        "Transport\n"
        "  Space: Play/Stop\n"
        "  R: Record\n"
        "  C: Toggle Metronome\n"
        "  /: Toggle Loop\n\n"
        "Workflow\n"
        "  Cmd/Ctrl+K: Command Palette\n"
        "  Cmd/Ctrl+S: Save\n"
        "  Cmd/Ctrl+E: Export\n"
        "  Cmd/Ctrl+I: Add Instrument Track\n"
        "  Cmd/Ctrl+T: Add Audio Track\n"
        "  Cmd/Ctrl+Shift+G: Quick Start Setup\n\n"
        "Panels\n"
        "  Alt+1: Inspector\n"
        "  Alt+2: Mixer\n"
        "  Alt+3: Piano Roll\n"
        "  Alt+4: Browser";

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Shortcut Cheatsheet", text);
}

void MainComponent::applyWorkspacePreset(Orion::WorkspacePreset preset)
{
    currentWorkspacePreset = preset;

    switch (preset)
    {
        case Orion::WorkspacePreset::Compose:
            if (browserWindow) browserWindow->setVisible(true); else browserPanel->setVisible(true);
            if (editorWindow) editorWindow->setVisible(true); else editorPanel->setVisible(true);
            if (mixerWindow) mixerWindow->setVisible(false); else mixerPanel->setVisible(false);
            if (inspectorWindow) inspectorWindow->setVisible(false); else inspectorPanel->setVisible(false);
            break;
        case Orion::WorkspacePreset::Arrange:
            if (browserWindow) browserWindow->setVisible(true); else browserPanel->setVisible(true);
            if (editorWindow) editorWindow->setVisible(false); else editorPanel->setVisible(false);
            if (mixerWindow) mixerWindow->setVisible(false); else mixerPanel->setVisible(false);
            if (inspectorWindow) inspectorWindow->setVisible(true); else inspectorPanel->setVisible(true);
            break;
        case Orion::WorkspacePreset::Mix:
            if (browserWindow) browserWindow->setVisible(false); else browserPanel->setVisible(false);
            if (editorWindow) editorWindow->setVisible(false); else editorPanel->setVisible(false);
            if (mixerWindow) mixerWindow->setVisible(true); else mixerPanel->setVisible(true);
            if (inspectorWindow) inspectorWindow->setVisible(true); else inspectorPanel->setVisible(true);
            break;
    }

    resized();
}

void MainComponent::showGuidedStartWizard(bool force)
{
    juce::ignoreUnused(force);
    if (state == Dashboard)
    {
        dashboard.createNewProject();
        return;
    }

    if (state != Editor)
        return;

    // Guided start is folded into a one-click quick-start action in v2.
    applyGuidedStartSetup(guidedStartState, true, true);
}

void MainComponent::applyGuidedStartSetup(const Orion::GuidedStartState& stateFromWizard, bool createStarterClip, bool onlyIfNoTracks)
{
    guidedStartState = stateFromWizard;
    guidedStartState.dismissed = true;
    forceShowGuidedStart = false;

    engine.setBpm(juce::jlimit(60.0f, 200.0f, guidedStartState.lastBpm));

    auto& appCmd = commandManager.getApplicationCommandManager();
    const bool shouldAddTrack = !onlyIfNoTracks || engine.getTracks().empty();
    if (shouldAddTrack)
    {
        if (guidedStartState.lastTrackType == Orion::GuidedStartTrackType::Audio)
            appCmd.invokeDirectly(Orion::CommandManager::AddAudioTrack, true);
        else
            appCmd.invokeDirectly(Orion::CommandManager::AddInstrumentTrack, true);

        if (guidedStartState.lastTrackType == Orion::GuidedStartTrackType::Instrument && createStarterClip)
            timeline.createStarterMidiClip();
    }

    const double sampleRate = engine.getSampleRate() > 0.0 ? engine.getSampleRate() : 44100.0;
    const double bpm = std::max(1.0f, engine.getBpm());
    const int bars = juce::jlimit(2, 32, guidedStartState.lastLoopBars);
    const double samplesPerBeat = (60.0 / bpm) * sampleRate;
    const uint64_t loopLength = (uint64_t)std::round(samplesPerBeat * 4.0 * (double)bars);
    engine.setLoopPoints(0, loopLength);
    engine.setLooping(true);

    Orion::UxTelemetry::logEvent(Orion::UxEventType::GuidedStartCompleted,
        {
            {"trackType", guidedStartState.lastTrackType == Orion::GuidedStartTrackType::Audio ? "audio" : "instrument"},
            {"bpm", guidedStartState.lastBpm},
            {"loopBars", guidedStartState.lastLoopBars}
        });

    resized();
}

void MainComponent::updateGuidedStartVisibility()
{
    guidedStartWizard.setVisible(false);
}

void MainComponent::trackUxEventProgress()
{
    auto transport = engine.getTransportState();
    const bool isRunning = (transport == Orion::TransportState::Playing || transport == Orion::TransportState::Recording);
    if (isRunning && !firstPlaybackLogged)
    {
        firstPlaybackLogged = true;
        Orion::UxTelemetry::logEvent(Orion::UxEventType::FirstPlayback);
    }
}

void MainComponent::showSettings()
{
    if (state == Dashboard) {
        dashboard.showSettingsTab();
        return;
    }

    if (settingsWindow)
    {
        settingsWindow->setVisible(true);
        settingsWindow->toFront(true);
        return;
    }

    class SettingsDialogWindow : public juce::DocumentWindow
    {
    public:
        SettingsDialogWindow() : DocumentWindow("Settings", juce::Colours::darkgrey, DocumentWindow::allButtons) {}
        void closeButtonPressed() override { setVisible(false); }
    };

    settingsWindow.reset(new SettingsDialogWindow());
    settingsWindow->setUsingNativeTitleBar(false);
    settingsWindow->setTitleBarHeight(0);

    auto content = std::make_unique<SettingsWindow>(deviceManager, [this](const juce::String& themeName) {
        setTheme(themeName);
    });

    auto wrapper = std::make_unique<WindowWrapper>(content.release(), "Settings", false, false);
    wrapper->setSize(500, 630);

    settingsWindow->setResizable(false, false);
    settingsWindow->setContentOwned(wrapper.release(), true);
    settingsWindow->centreWithSize(500, 630);
    settingsWindow->setVisible(true);
}

void MainComponent::showAbout()
{
    if (aboutWindow)
    {
        aboutWindow->setVisible(true);
        aboutWindow->toFront(true);
        return;
    }

    class AboutDialogWindow : public juce::DocumentWindow
    {
    public:
        AboutDialogWindow() : DocumentWindow("About Orion", juce::Colours::darkgrey, DocumentWindow::allButtons) {}
        void closeButtonPressed() override { setVisible(false); }
    };

    aboutWindow.reset(new AboutDialogWindow());
    aboutWindow->setUsingNativeTitleBar(false);
    aboutWindow->setTitleBarHeight(0);

    auto content = std::make_unique<AboutComponent>();
    auto wrapper = std::make_unique<WindowWrapper>(content.release(), "About Orion", false, false);
    wrapper->setSize(600, 400);

    aboutWindow->setResizable(false, false);
    aboutWindow->setContentOwned(wrapper.release(), true);
    aboutWindow->centreWithSize(600, 400);
    aboutWindow->setVisible(true);
}

void MainComponent::showDocumentation()
{
    if (documentationWindow)
    {
        documentationWindow->setVisible(true);
        documentationWindow->toFront(true);
        return;
    }

    documentationView = std::make_unique<Orion::DocumentationViewComponent>();
    documentationView->setSize(800, 600);


    documentationWindow = std::make_unique<DetachedWindow>("Documentation", documentationView.get(), [this] {
        if (documentationWindow) documentationWindow->setVisible(false);
    });
}
