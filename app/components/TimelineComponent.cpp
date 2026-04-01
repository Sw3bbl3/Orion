#include "TimelineComponent.h"
#include "../OrionLookAndFeel.h"
#include "AutomationLaneComponent.h"
#include "orion/AudioClip.h"
#include "orion/InstrumentTrack.h"
#include "orion/MidiClip.h"
#include "orion/EditorCommands.h"
#include "orion/SettingsManager.h"
#include "orion/PluginManager.h"
#include "ClipComponent.h"
#include "MidiClipComponent.h"
#include "../OrionIcons.h"
#include "../UiHints.h"
#include <algorithm>
#include <unordered_map>


namespace {
    struct AutomationLaneUIState {
        bool showVolume = true;
        bool showPan = false;
        Orion::AutomationParameter activeParamType = Orion::AutomationParameter::Volume;
    };

    std::unordered_map<const Orion::AudioTrack*, AutomationLaneUIState> automationLaneUiStateByTrack;

    AutomationLaneUIState getAutomationLaneState(const std::shared_ptr<Orion::AudioTrack>& track)
    {
        if (!track) return {};

        auto& state = automationLaneUiStateByTrack[track.get()];
        if (!state.showVolume && !state.showPan) {
            state.showVolume = true;
            state.activeParamType = Orion::AutomationParameter::Volume;
        }
        if (state.activeParamType == Orion::AutomationParameter::Volume && !state.showVolume) state.activeParamType = Orion::AutomationParameter::Pan;
        if (state.activeParamType == Orion::AutomationParameter::Pan && !state.showPan) state.activeParamType = Orion::AutomationParameter::Volume;
        return state;
    }

    void setAutomationLaneState(const std::shared_ptr<Orion::AudioTrack>& track, const AutomationLaneUIState& state)
    {
        if (!track) return;
        automationLaneUiStateByTrack[track.get()] = state;
    }

    int getVisibleAutomationLaneCount(const std::shared_ptr<Orion::AudioTrack>& track)
    {
        if (!track || !track->isAutomationVisible()) return 0;
        auto state = getAutomationLaneState(track);
        return (state.showVolume ? 1 : 0) + (state.showPan ? 1 : 0);
    }

    int getAutomationExtraHeight(const std::shared_ptr<Orion::AudioTrack>& track)
    {
        return getVisibleAutomationLaneCount(track) * 100;
    }

    float getBpmAtFrame(Orion::AudioEngine& engine, uint64_t frame)
    {
        float bpm = engine.getBpm();
        if (bpm <= 0.1f) bpm = 120.0f;

        auto tempos = engine.getTempoMarkers();
        for (auto it = tempos.rbegin(); it != tempos.rend(); ++it) {
            if (it->position <= frame) {
                bpm = it->bpm;
                if (bpm <= 0.1f) bpm = 120.0f;
                break;
            }
        }
        return bpm;
    }
}

class TimelineEmptyStateOverlay : public juce::Component
{
public:
    TimelineEmptyStateOverlay()
    {
        setInterceptsMouseClicks(false, false);
        setAlwaysOnTop(true);

        titleLabel.setText("Start Something New", juce::dontSendNotification);
        titleLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel);

        subtitleLabel.setText("Add a track or drop audio files here to begin.", juce::dontSendNotification);
        subtitleLabel.setFont(juce::FontOptions(13.5f, juce::Font::plain));
        subtitleLabel.setJustificationType(juce::Justification::centred);
        subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
        addAndMakeVisible(subtitleLabel);

        stylePrimary(addAudioButton);
        addAudioButton.setButtonText("Add Audio Track");
        addAudioButton.onClick = [this] { if (onAddAudio) onAddAudio(); };
        addAndMakeVisible(addAudioButton);

        stylePrimary(addInstrumentButton);
        addInstrumentButton.setButtonText("Add Instrument Track");
        addInstrumentButton.onClick = [this] { if (onAddInstrument) onAddInstrument(); };
        addAndMakeVisible(addInstrumentButton);

        styleSecondary(importButton);
        importButton.setButtonText("Import Audio...");
        importButton.onClick = [this] { if (onImportAudio) onImportAudio(); };
        addAndMakeVisible(importButton);

        styleLink(guidedStartButton);
        guidedStartButton.setButtonText("Start Making Music");
        guidedStartButton.onClick = [this] { if (onShowGuidedStart) onShowGuidedStart(); };
        addAndMakeVisible(guidedStartButton);
    }

    void setGuidedStartVisible(bool visible)
    {
        guidedStartButton.setVisible(visible);
    }

    void setOverlayVisible(bool visible)
    {
        setVisible(visible);
        setEnabled(visible);
        setInterceptsMouseClicks(visible, visible);
    }

    void paint(juce::Graphics& g) override
    {
        auto bg = findColour(OrionLookAndFeel::timelineBackgroundColourId).withAlpha(0.85f);
        g.setColour(bg);
        g.fillAll();

        auto card = getLocalBounds().toFloat().reduced(20.0f);
        card = card.withSizeKeepingCentre(440.0f, 240.0f);

        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(card, 12.0f);

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(card, 12.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);
        auto card = area.withSizeKeepingCentre(440, 240).reduced(20);

        titleLabel.setBounds(card.removeFromTop(32));
        card.removeFromTop(6);
        subtitleLabel.setBounds(card.removeFromTop(22));

        card.removeFromTop(16);
        addAudioButton.setBounds(card.removeFromTop(30));
        card.removeFromTop(8);
        addInstrumentButton.setBounds(card.removeFromTop(30));
        card.removeFromTop(8);
        importButton.setBounds(card.removeFromTop(28));

        card.removeFromTop(10);
        guidedStartButton.setBounds(card.removeFromTop(24));
    }

    std::function<void()> onAddAudio;
    std::function<void()> onAddInstrument;
    std::function<void()> onImportAudio;
    std::function<void()> onShowGuidedStart;

private:
    void stylePrimary(juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0A84FF));
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }

    void styleSecondary(juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::black.withAlpha(0.25f));
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.9f));
    }

    void styleLink(juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF0A84FF));
    }

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::TextButton addAudioButton;
    juce::TextButton addInstrumentButton;
    juce::TextButton importButton;
    juce::TextButton guidedStartButton;
};

double GlobalPlayheadOverlay::frameToX(uint64_t targetFrame) const
{
    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) return 0.0;

    auto tempos = engine.getTempoMarkers();
    auto sigs = engine.getTimeSigMarkers();

    uint64_t currentFrame = 0;
    double currentX = 0.0;

    float currentBpm = engine.getBpm();
    int currentNum = 4;
    int currentDen = 4;
    engine.getTimeSignature(currentNum, currentDen);

    size_t tIdx = 0;
    size_t sIdx = 0;

    while (tIdx < tempos.size() && tempos[tIdx].position <= 0) {
        currentBpm = tempos[tIdx].bpm;
        tIdx++;
    }
    while (sIdx < sigs.size() && sigs[sIdx].position <= 0) {
        currentNum = sigs[sIdx].numerator;
        currentDen = sigs[sIdx].denominator;
        sIdx++;
    }

    while (currentFrame < targetFrame)
    {
        while (tIdx < tempos.size() && tempos[tIdx].position <= currentFrame) {
            currentBpm = tempos[tIdx].bpm;
            tIdx++;
        }
        while (sIdx < sigs.size() && sigs[sIdx].position <= currentFrame) {
            currentNum = sigs[sIdx].numerator;
            currentDen = sigs[sIdx].denominator;
            sIdx++;
        }

        double quartersPerBar = (double)currentNum * (4.0 / (double)currentDen);
        double secondsPerBar = quartersPerBar * (60.0 / currentBpm);
        uint64_t framesPerBar = (uint64_t)(secondsPerBar * sampleRate);
        if (framesPerBar == 0) framesPerBar = 1;

        if (currentFrame + framesPerBar > targetFrame) {
            double fraction = (double)(targetFrame - currentFrame) / (double)framesPerBar;
            return currentX + (fraction * (secondsPerBar * pixelsPerSecond));
        }

        currentX += secondsPerBar * pixelsPerSecond;
        currentFrame += framesPerBar;
    }

    return currentX;
}

void GlobalPlayheadOverlay::paint(juce::Graphics& g)
{
    if (engine.getSampleRate() <= 0)
        return;

    double playheadPos = frameToX(engine.getCursor());
    float x = (float)playheadPos - (float)currentScrollX;

    if (x >= -10 && x <= (float)getWidth() + 10)
    {
        g.setColour(juce::Colours::cyan);
        g.drawLine(x, 0, x, (float)getHeight(), 1.0f);

        juce::Path p;
        p.addTriangle(x - 6, 0, x + 6, 0, x, 12);
        g.fillPath(p);
    }
}

class TrackHeaderComponent : public juce::Component, public juce::Timer
{
public:
    TrackHeaderComponent(Orion::AudioEngine& e, std::shared_ptr<Orion::AudioTrack> t, Orion::CommandHistory& history)
        : engine(e), track(t), commandHistory(history)
    {
        if (track) {
            const float* c = track->getColor();
            lastColor[0] = c[0]; lastColor[1] = c[1]; lastColor[2] = c[2];
        }
        addAndMakeVisible(nameLabel);
        nameLabel.setText(track->getName(), juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centredLeft);
        nameLabel.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));

        addAndMakeVisible(volumeSlider);
        volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volumeSlider.setRange(0.0, 1.0);
        volumeSlider.setDoubleClickReturnValue(true, 1.0);
        volumeSlider.setValue(track->getVolume(), juce::dontSendNotification);
        volumeSlider.onValueChange = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackVolumeCommand>(
                track, (float)volumeSlider.getValue(), track->getVolume()
            ));
        };
        volumeSlider.setTooltip("Volume");
        volumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.72f));
        volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::black.withAlpha(0.35f));
        volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.95f));

        addAndMakeVisible(panKnob);
        panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panKnob.setRange(-1.0, 1.0, 0.01);
        panKnob.setDoubleClickReturnValue(true, 0.0);
        panKnob.setValue(track->getPan(), juce::dontSendNotification);
        panKnob.onValueChange = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackPanCommand>(
                track, (float)panKnob.getValue(), track->getPan()
            ));
        };
        panKnob.setTooltip("Pan");
        panKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white.withAlpha(0.82f));
        panKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black.withAlpha(0.55f));
        panKnob.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.95f));


        setupTextButton(muteButton, "M", juce::Colour(0xFFFF453A), "Mute");
        setupTextButton(soloButton, "S", juce::Colour(0xFFFFD60A), "Solo");
        setupTextButton(recButton, "R", juce::Colour(0xFFFF453A), "Record Arm");
        setupTextButton(inputButton, "I", juce::Colour(0xFF0A84FF), "Input Monitor");
        setupTextButton(autoButton, "A", juce::Colour(0xFF30D158), "Automation Read/Write");
        setupTextButton(laneButton, "V", juce::Colour(0xFF0A84FF), "Automation Lane");

        setupTextButton(deleteButton, "X", juce::Colour(0xFF888888), "Delete Track");
        laneButton->setClickingTogglesState(false);
        deleteButton->setClickingTogglesState(false);

        muteButton->onClick = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackMuteCommand>(track, muteButton->getToggleState()));
        };
        soloButton->onClick = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackSoloCommand>(engine, track, soloButton->getToggleState()));
        };
        recButton->onClick = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackArmCommand>(track, recButton->getToggleState()));
        };
        autoButton->onClick = [this] {
            track->setShowAutomation(autoButton->getToggleState());
            if (onHeightChanged) onHeightChanged();
        };
        laneButton->onClick = [this] { showAutomationLaneMenu(); };
        deleteButton->onClick = [this] { if (onDeleted) onDeleted(); };

        // Ensure mouse clicks pass through for right-click handling on background
        // but allow child components (like the text editor) to receive clicks
        nameLabel.setInterceptsMouseClicks(false, true);

        nameLabel.onTextChange = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackNameCommand>(
                track, nameLabel.getText().toStdString(), track->getName()
            ));
        };

        updateState();
        startTimerHz(15);
    }

    void setSelected(bool s)
    {
        if (selected == s) return;
        selected = s;
        repaint();
    }

    std::function<void()> onSelected;
    std::function<void()> onDeleted;
    std::function<void()> onDuplicate;
    std::function<void()> onSelectAll;
    std::function<void()> onHeightChanged;
    std::function<void(juce::Point<int>)> onDragStarted;
    std::function<void(juce::Point<int>)> onDragMoved;
    std::function<void(juce::Point<int>)> onDragEnded;

    std::shared_ptr<Orion::AudioTrack> getTrack() const { return track; }

    void mouseDown(const juce::MouseEvent& e) override
    {
        isHeaderDragging = false;
        if (e.mods.isRightButtonDown())
        {
            showContextMenu();
            return;
        }

        if (onSelected) onSelected();
        juce::Component::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
            return;

        if (!isHeaderDragging)
        {
            if (e.getDistanceFromDragStart() < 4)
                return;

            isHeaderDragging = true;
            if (onDragStarted) onDragStarted(e.getScreenPosition());
        }

        if (onDragMoved) onDragMoved(e.getScreenPosition());
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (isHeaderDragging && onDragEnded)
            onDragEnded(e.getScreenPosition());

        isHeaderDragging = false;
    }

    void showContextMenu()
    {
        juce::PopupMenu m;
        m.addItem("Rename", [this] {
            nameLabel.setEditable(true, false, false);
            nameLabel.showEditor();
        });
        m.addItem("Duplicate", [this] { if (onDuplicate) onDuplicate(); });
        m.addItem("Select All Clips", [this] { if (onSelectAll) onSelectAll(); });

        m.addSeparator();
        m.addItem("Input Monitor", [this] {
            // Toggle logic if we had one, for now just a placeholder for the UI
        });
        m.addItem("Show Automation", true, track->isAutomationVisible(), [this] {
            track->setShowAutomation(!track->isAutomationVisible());
            if (onHeightChanged) onHeightChanged();
        });

        juce::PopupMenu colorMenu;
        colorMenu.addItem("Red", [this] { track->setColor(0.8f, 0.2f, 0.2f); repaint(); });
        colorMenu.addItem("Green", [this] { track->setColor(0.2f, 0.8f, 0.2f); repaint(); });
        colorMenu.addItem("Blue", [this] { track->setColor(0.2f, 0.2f, 0.8f); repaint(); });
        colorMenu.addItem("Yellow", [this] { track->setColor(0.8f, 0.8f, 0.2f); repaint(); });
        colorMenu.addItem("Purple", [this] { track->setColor(0.6f, 0.2f, 0.8f); repaint(); });
        colorMenu.addItem("Grey", [this] { track->setColor(0.5f, 0.5f, 0.5f); repaint(); });

        m.addSubMenu("Color", colorMenu);

        m.addSeparator();
        m.addItem("Reset Volume", [this] {
            track->setVolume(1.0f);
            volumeSlider.setValue(1.0f, juce::dontSendNotification);
        });
        m.addItem("Reset Pan", [this] { track->setPan(0.0f); });

        m.addSeparator();
        m.addItem("Delete", [this] { if (onDeleted) onDeleted(); });
        m.showMenuAsync(juce::PopupMenu::Options());
    }

    void showAutomationLaneMenu()
    {
        auto state = getAutomationLaneState(track);

        juce::PopupMenu m;
        m.addItem("Show Volume Lane", true, state.showVolume, [this, state] {
            auto next = state;
            next.showVolume = !next.showVolume;
            if (!next.showVolume && !next.showPan) next.showVolume = true;
            if (next.activeParamType == Orion::AutomationParameter::Volume && !next.showVolume) next.activeParamType = Orion::AutomationParameter::Pan;
            setAutomationLaneState(track, next);
            if (onHeightChanged) onHeightChanged();
            updateState();
        });
        m.addItem("Show Pan Lane", true, state.showPan, [this, state] {
            auto next = state;
            next.showPan = !next.showPan;
            if (!next.showVolume && !next.showPan) next.showPan = true;
            if (next.activeParamType == Orion::AutomationParameter::Pan && !next.showPan) next.activeParamType = Orion::AutomationParameter::Volume;
            setAutomationLaneState(track, next);
            if (onHeightChanged) onHeightChanged();
            updateState();
        });

        m.addSeparator();
        m.addItem("Edit Volume Points", true, state.showVolume, [this] {
            auto next = getAutomationLaneState(track);
            next.showVolume = true;
            next.activeParamType = Orion::AutomationParameter::Volume;
            setAutomationLaneState(track, next);
            if (onHeightChanged) onHeightChanged();
            updateState();
        });
        m.addItem("Edit Pan Points", true, state.showPan, [this] {
            auto next = getAutomationLaneState(track);
            next.showPan = true;
            next.activeParamType = Orion::AutomationParameter::Pan;
            setAutomationLaneState(track, next);
            if (onHeightChanged) onHeightChanged();
            updateState();
        });

        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(laneButton.get()));
    }

    void timerCallback() override
    {
        updateState();

        if (track) {
            const float* c = track->getColor();
            if (std::abs(c[0] - lastColor[0]) > 0.001f ||
                std::abs(c[1] - lastColor[1]) > 0.001f ||
                std::abs(c[2] - lastColor[2]) > 0.001f)
            {
                lastColor[0] = c[0]; lastColor[1] = c[1]; lastColor[2] = c[2];
                repaint();
            }
        }
    }

    void updateState()
    {
        if (muteButton) muteButton->setToggleState(track->isMuted(), juce::dontSendNotification);
        if (soloButton) soloButton->setToggleState(track->isSolo(), juce::dontSendNotification);
        if (recButton) recButton->setToggleState(track->isArmed(), juce::dontSendNotification);
        if (autoButton) autoButton->setToggleState(track->isAutomationVisible(), juce::dontSendNotification);

        auto laneState = getAutomationLaneState(track);
        if (laneButton) {
            laneButton->setEnabled(track->isAutomationVisible());
            laneButton->setButtonText(laneState.activeParamType == Orion::AutomationParameter::Volume ? "V" : "P");
            laneButton->setTooltip(laneState.activeParamType == Orion::AutomationParameter::Volume ? "Editing Volume Automation" : "Editing Pan Automation");
        }

        if (std::abs(volumeSlider.getValue() - track->getVolume()) > 0.01) {
             volumeSlider.setValue(track->getVolume(), juce::dontSendNotification);
        }

        if (std::abs(panKnob.getValue() - track->getPan()) > 0.01) {
             panKnob.setValue(track->getPan(), juce::dontSendNotification);
        }

        if (nameLabel.getText() != juce::String(track->getName()))
            nameLabel.setText(track->getName(), juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        juce::Colour bg = findColour(OrionLookAndFeel::trackHeaderBackgroundColourId).interpolatedWith(juce::Colours::black, 0.25f);
        const float* color = track->getColor();
        juce::Colour trackColor = juce::Colour::fromFloatRGBA(color[0], color[1], color[2], 1.0f);
        const float corner = 7.0f;

        g.setColour(bg);
        g.fillRoundedRectangle(bounds.reduced(0.5f), corner);

        float alpha = Orion::SettingsManager::get().fullTrackColoring ? 0.34f : 0.10f;
        g.setColour(trackColor.withAlpha(alpha));
        g.fillRoundedRectangle(bounds.reduced(0.5f), corner);

        if (selected)
        {
            auto accent = findColour(OrionLookAndFeel::accentColourId);
            g.setColour(accent.withAlpha(0.14f));
            g.fillRoundedRectangle(bounds.reduced(1.0f), corner - 1.0f);

            g.setColour(accent.withAlpha(0.95f));
            g.drawRoundedRectangle(bounds.reduced(0.75f), corner - 0.5f, 2.0f);
        }

        g.setColour(trackColor.withAlpha(0.92f));
        g.fillRoundedRectangle(1.0f, 1.0f, 4.0f, (float)getHeight() - 2.0f, 2.0f);

        juce::ColourGradient topGlow(juce::Colours::white.withAlpha(0.08f), 0, 0, juce::Colours::transparentBlack, 0, (float)getHeight() * 0.65f, false);
        g.setGradientFill(topGlow);
        g.fillRoundedRectangle(bounds.reduced(1.0f), corner - 1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);

        g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId).withAlpha(0.55f));
        g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());

        if (track->isArmed()) {
            auto recBounds = bounds.reduced(5.0f, 2.0f);
            g.setColour(findColour(OrionLookAndFeel::timelineRecordingRegionColourId).withAlpha(0.10f));
            g.fillRoundedRectangle(recBounds, corner - 2.0f);

            g.setColour(findColour(OrionLookAndFeel::timelineRecordingRegionColourId).withAlpha(0.46f));
            g.drawRoundedRectangle(recBounds, corner - 2.0f, 1.0f);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 7);
        area.removeFromLeft(4);

        auto topRow = area.removeFromTop(22);
        if (deleteButton) deleteButton->setBounds(topRow.removeFromRight(18).reduced(0, 1));
        nameLabel.setBounds(topRow.reduced(0, 1));

        area.removeFromTop(6);

        auto btnRow = area.removeFromTop(20);
        constexpr int btnW = 21;
        constexpr int gap = 3;
        if (muteButton) muteButton->setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(gap);
        if (soloButton) soloButton->setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(gap);
        if (recButton) recButton->setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(gap + 2);
        if (autoButton) autoButton->setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(gap);
        if (laneButton) laneButton->setBounds(btnRow.removeFromLeft(btnW));
        if (inputButton) inputButton->setVisible(false);

        area.removeFromTop(7);

        auto ctrlRow = area.removeFromTop(26);
        panKnob.setBounds(ctrlRow.removeFromRight(26));
        ctrlRow.removeFromRight(6);
        volumeSlider.setBounds(ctrlRow.reduced(0, 4));
    }

private:
    Orion::AudioEngine& engine;
    std::shared_ptr<Orion::AudioTrack> track;
    Orion::CommandHistory& commandHistory;
    float lastColor[3] = { 0.0f, 0.0f, 0.0f };
    bool selected = false;
    juce::Label nameLabel;
    juce::Slider volumeSlider;
    juce::Slider panKnob;
    std::unique_ptr<juce::TextButton> muteButton;
    std::unique_ptr<juce::TextButton> soloButton;
    std::unique_ptr<juce::TextButton> recButton;
    std::unique_ptr<juce::TextButton> inputButton;
    std::unique_ptr<juce::TextButton> autoButton;
    std::unique_ptr<juce::TextButton> laneButton;
    std::unique_ptr<juce::TextButton> deleteButton;
    bool isHeaderDragging = false;

    void setupTextButton(std::unique_ptr<juce::TextButton>& btn, const juce::String& text, juce::Colour onColour, const juce::String& tooltip)
    {
        btn = std::make_unique<juce::TextButton>(text);
        btn->setTooltip(tooltip);
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.83f));
        btn->setColour(juce::TextButton::textColourOnId, juce::Colours::white.withAlpha(0.98f));
        btn->setColour(juce::TextButton::buttonOnColourId, onColour);
        btn->setColour(juce::TextButton::buttonColourId, juce::Colours::black.withAlpha(0.36f));
        btn->setClickingTogglesState(true);
        addAndMakeVisible(btn.get());
    }

    void lookAndFeelChanged() override
    {
        nameLabel.setColour(juce::Label::textColourId, findColour(juce::Label::textColourId));
        repaint();
    }
};

class TimelineLaneComponent : public juce::Component, public juce::Timer
{
public:
    TimelineLaneComponent(std::shared_ptr<Orion::AudioTrack> t, Orion::AudioEngine& e, double pps,
                          std::function<uint64_t(uint64_t)> snapFn,
                          std::function<TimelineTool()> toolGetterFn)
        : track(t), engine(e), pixelsPerSecond(pps), snapFunc(snapFn), toolGetter(toolGetterFn)
    {
        setWantsKeyboardFocus(true);

        rebuildAutomationLanes();

        updateClips();
        if (track) {
            const float* c = track->getColor();
            lastColor[0] = c[0]; lastColor[1] = c[1]; lastColor[2] = c[2];
        }
        startTimerHz(15);
    }

    void timerCallback() override
    {
        if (track) {
            const float* c = track->getColor();
            if (std::abs(c[0] - lastColor[0]) > 0.001f ||
                std::abs(c[1] - lastColor[1]) > 0.001f ||
                std::abs(c[2] - lastColor[2]) > 0.001f)
            {
                lastColor[0] = c[0]; lastColor[1] = c[1]; lastColor[2] = c[2];

                for (auto& cc : clipComponents) cc->repaint();
                for (auto& mc : midiClipComponents) mc->repaint();
                repaint();
            }
        }
    }

    std::function<void(const juce::MouseEvent&)> onBackgroundMouseDown;
    std::function<void(const juce::MouseEvent&)> onBackgroundMouseDrag;
    std::function<void(const juce::MouseEvent&)> onBackgroundMouseUp;

    void mouseDown(const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();
        if (onBackgroundMouseDown) onBackgroundMouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (onBackgroundMouseDrag) onBackgroundMouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (onBackgroundMouseUp) onBackgroundMouseUp(e);
    }

    void setSelectionState(const std::function<bool(std::shared_ptr<Orion::AudioClip>)>& isSelectedAudio,
                           const std::function<bool(std::shared_ptr<Orion::MidiClip>)>& isSelectedMidi)
    {
        for (auto& cc : clipComponents) {
            if (auto c = cc->getClip()) cc->setSelected(isSelectedAudio(c));
        }
        for (auto& mc : midiClipComponents) {
            if (auto c = mc->getClip()) mc->setSelected(isSelectedMidi(c));
        }
    }

    void setPixelsPerSecond(double pps)
    {
        pixelsPerSecond = pps;
        for (auto& entry : automationLanes)
            entry.component->setPixelsPerSecond(pps);
        resized();
    }

    std::function<void(std::shared_ptr<Orion::AudioTrack>, std::shared_ptr<Orion::AudioClip>, bool)> onClipSelected;
    std::function<void(std::shared_ptr<Orion::AudioTrack>, std::shared_ptr<Orion::MidiClip>, bool)> onMidiClipSelected;
    std::function<void(std::shared_ptr<Orion::AudioTrack>, std::shared_ptr<Orion::MidiClip>)> onMidiClipOpened;

    std::shared_ptr<Orion::AudioTrack> getTrack() const { return track; }

    void updateClips()
    {
        for (auto& cc : clipComponents) removeChildComponent(cc.get());
        for (auto& mc : midiClipComponents) removeChildComponent(mc.get());
        clipComponents.clear();
        midiClipComponents.clear();

        if (!track) return;


        auto* instTrack = dynamic_cast<Orion::InstrumentTrack*>(track.get());
        if (instTrack)
        {
            auto midiClips = instTrack->getMidiClips();
            if (midiClips)
            {
                for (auto& clip : *midiClips)
                {
                    auto comp = std::make_unique<MidiClipComponent>(clip);

                    comp->snapFunction = snapFunc;

                    comp->isSplitToolActive = [this] {
                        return toolGetter && toolGetter() == TimelineTool::Split;
                    };

                    comp->onSelectionChanged = [this, clip](MidiClipComponent* c) {
                        juce::ignoreUnused(c);
                        bool mod = juce::ModifierKeys::getCurrentModifiers().isCommandDown() || juce::ModifierKeys::getCurrentModifiers().isShiftDown();
                        if (onMidiClipSelected) onMidiClipSelected(track, clip, mod);
                    };

                    comp->onDrag = [this](MidiClipComponent* c, int deltaX) {
                        if (pixelsPerSecond <= 0) return;
                        double secondsDelta = (double)deltaX / pixelsPerSecond;
                        double sr = engine.getSampleRate();
                        if (sr <= 0) sr = 44100.0;
                        int64_t frameDelta = (int64_t)(secondsDelta * sr);
                        auto clip = c->getClip();
                        if (!clip) return;
                        int64_t currentStart = (int64_t)clip->getStartFrame();
                        int64_t newStart = currentStart + frameDelta;
                        if (newStart < 0) newStart = 0;
                        if (snapFunc) newStart = snapFunc((uint64_t)newStart);
                        clip->setStartFrame((uint64_t)newStart);
                        resized();
                        repaint();
                    };

                    comp->onDoubleClick = [this](MidiClipComponent* c) {
                         auto clip = c->getClip();
                         if (!clip) return;
                         if (onMidiClipOpened) onMidiClipOpened(track, clip);
                    };

                    comp->onSplit = [this, instTrack](MidiClipComponent* c, const juce::MouseEvent& e) -> bool {
                        if (toolGetter && toolGetter() == TimelineTool::Split) {
                            if (pixelsPerSecond <= 0.0) return false;
                            auto pt = e.getEventRelativeTo(this).position;
                            double time = pt.x / pixelsPerSecond;
                            double sr = engine.getSampleRate();
                            if (sr <= 0.0) sr = 44100.0;
                            uint64_t frame = (uint64_t)(time * sr);

                            auto clip = c->getClip();
                            if (!clip) return false;
                            auto newClip = clip->split(frame);
                            if (newClip) {
                                instTrack->addMidiClip(newClip);
                                updateClips();
                                resized();
                                repaint();
                                return true;
                            }
                        }
                        return false;
                    };

                    comp->onDelete = [this, clip, instTrack](MidiClipComponent* c) {
                        juce::ignoreUnused(c);
                        instTrack->removeMidiClip(clip);
                        updateClips();
                        resized();
                    };

                    comp->onRename = [this](MidiClipComponent* c, juce::String n) {
                        juce::ignoreUnused(c, n);
                        repaint();
                    };

                    comp->onResizing = [this](MidiClipComponent* c) {
                        juce::ignoreUnused(c);
                        resized();
                    };

                    addAndMakeVisible(comp.get());
                    midiClipComponents.push_back(std::move(comp));
                }
            }
        }


        auto clips = track->getClips();
        if (clips)
        {
            for (auto& clip : *clips)
            {
                auto comp = std::make_unique<ClipComponent>(clip);

                comp->snapFunction = snapFunc;

                comp->isSplitToolActive = [this] {
                    return toolGetter && toolGetter() == TimelineTool::Split;
                };

                comp->onSelectionChanged = [this, clip](ClipComponent* c) {
                    juce::ignoreUnused(c);
                    bool mod = juce::ModifierKeys::getCurrentModifiers().isCommandDown() || juce::ModifierKeys::getCurrentModifiers().isShiftDown();
                    if (onClipSelected) onClipSelected(track, clip, mod);
                };

                comp->onDrag = [this](ClipComponent* c, int deltaX) {
                    if (pixelsPerSecond <= 0) return;
                    double secondsDelta = (double)deltaX / pixelsPerSecond;
                    double sr = engine.getSampleRate();
                    if (sr <= 0) sr = 44100.0;
                    int64_t frameDelta = (int64_t)(secondsDelta * sr);
                    auto clip = c->getClip();
                    if (!clip) return;
                    int64_t currentStart = (int64_t)clip->getStartFrame();
                    int64_t newStart = currentStart + frameDelta;
                    if (newStart < 0) newStart = 0;
                        if (snapFunc) newStart = snapFunc((uint64_t)newStart);
                    clip->setStartFrame((uint64_t)newStart);
                    resized();
                    repaint();
                };

                comp->onDragEnd = [this](ClipComponent* c) {
                     juce::ignoreUnused(c);
                     track->resolveOverlaps();
                     repaint();
                };

                comp->onSplit = [this](ClipComponent* c, const juce::MouseEvent& e) -> bool {
                    if (toolGetter && toolGetter() == TimelineTool::Split) {
                        if (pixelsPerSecond <= 0.0) return false;
                        auto pt = e.getEventRelativeTo(this).position;
                        double time = pt.x / pixelsPerSecond;
                        double sr = engine.getSampleRate();
                        if (sr <= 0.0) sr = 44100.0;
                        uint64_t frame = (uint64_t)(time * sr);

                        auto clip = c->getClip();
                        if (!clip) return false;
                        auto newClip = clip->split(frame);
                        if (newClip) {
                            track->addClip(newClip);
                            updateClips();
                            resized();
                            repaint();
                            return true;
                        }
                    }
                    return false;
                };

                comp->onDelete = [this, clip](ClipComponent* c) {
                    juce::ignoreUnused(c);
                    track->removeClip(clip);
                    updateClips();
                    resized();
                };

                comp->onRename = [this](ClipComponent* c, juce::String n) {
                     juce::ignoreUnused(c, n);
                     repaint();
                };

                comp->onResizing = [this](ClipComponent* c) {
                     juce::ignoreUnused(c);
                     resized();
                };

                addAndMakeVisible(comp.get());
                clipComponents.push_back(std::move(comp));
            }
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        auto* instTrack = dynamic_cast<Orion::InstrumentTrack*>(track.get());
        if (instTrack)
        {
            if (pixelsPerSecond <= 0.0)
                return;

            double clickTime = (double)e.x / pixelsPerSecond;
            double sr = engine.getSampleRate();
            if (sr <= 0) sr = 44100.0;

            auto clip = std::make_shared<Orion::MidiClip>();
            clip->setStartFrame((uint64_t)(clickTime * sr));
            clip->setLengthFrames((uint64_t)(sr * 4.0));
            clip->setName("MIDI Clip");

            instTrack->addMidiClip(clip);
            updateClips();
            resized();

            if (onMidiClipSelected) onMidiClipSelected(track, clip, false);
            if (onMidiClipOpened) onMidiClipOpened(track, clip);
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(findColour(OrionLookAndFeel::timelineGridColourId));
        g.drawLine(0, (float)getHeight(), (float)getWidth(), (float)getHeight());
    }

    void resized() override
    {
        double sr = engine.getSampleRate();
        if (sr <= 0) sr = 44100.0;

        int clipAreaH = getHeight();
        const int laneCount = (int)automationLanes.size();
        if (laneCount > 0) {
            const int desiredTotalAutoH = laneCount * 100;
            if (getHeight() > desiredTotalAutoH + 20)
                clipAreaH = getHeight() - desiredTotalAutoH;
            else
                clipAreaH = getHeight() / 2;

            int laneY = clipAreaH;
            int remainingH = getHeight() - clipAreaH;
            for (int i = 0; i < laneCount; ++i) {
                const int laneH = remainingH / (laneCount - i);
                automationLanes[(size_t)i].component->setBounds(0, laneY, getWidth(), laneH);
                laneY += laneH;
                remainingH -= laneH;
            }
        }

        for (auto& cc : clipComponents)
        {
            auto clip = cc->getClip();
            if (!clip) continue;
            double startSeconds = (double)clip->getStartFrame() / sr;
            double durSeconds = (double)clip->getLengthFrames() / sr;
            int x = (int)(startSeconds * pixelsPerSecond);
            int w = (int)(durSeconds * pixelsPerSecond);
            if (w < 2) w = 2;
            cc->setBounds(x, 1, w, clipAreaH - 2);
        }

        for (auto& mc : midiClipComponents)
        {
            auto clip = mc->getClip();
            if (!clip) continue;
            double startSeconds = (double)clip->getStartFrame() / sr;
            double durSeconds = (double)clip->getLengthFrames() / sr;
            int x = (int)(startSeconds * pixelsPerSecond);
            int w = (int)(durSeconds * pixelsPerSecond);
            if (w < 2) w = 2;
            mc->setBounds(x, 1, w, clipAreaH - 2);
        }
    }

private:
    std::shared_ptr<Orion::AudioTrack> track;
    Orion::AudioEngine& engine;
    double pixelsPerSecond;
    float lastColor[3] = {0.0f, 0.0f, 0.0f};
    std::function<uint64_t(uint64_t)> snapFunc;
    std::function<TimelineTool()> toolGetter;
    std::vector<std::unique_ptr<ClipComponent>> clipComponents;
    std::vector<std::unique_ptr<MidiClipComponent>> midiClipComponents;
    struct AutomationLaneEntry {
        Orion::AutomationParameter paramType = Orion::AutomationParameter::Volume;
        std::unique_ptr<AutomationLaneComponent> component;
    };
    std::vector<AutomationLaneEntry> automationLanes;

    void rebuildAutomationLanes()
    {
        for (auto& entry : automationLanes) removeChildComponent(entry.component.get());
        automationLanes.clear();

        if (!track || !track->isAutomationVisible()) return;

        const auto laneState = getAutomationLaneState(track);
        if (laneState.showVolume) {
            AutomationLaneEntry lane;
            lane.paramType = Orion::AutomationParameter::Volume;
            lane.component = std::make_unique<AutomationLaneComponent>(track, Orion::AutomationParameter::Volume);
            lane.component->setPixelsPerSecond(pixelsPerSecond);
            lane.component->setSampleRate(engine.getSampleRate());
            lane.component->setInterceptsMouseClicks(laneState.activeParamType == Orion::AutomationParameter::Volume, false);
            addAndMakeVisible(lane.component.get());
            automationLanes.push_back(std::move(lane));
        }

        if (laneState.showPan) {
            AutomationLaneEntry lane;
            lane.paramType = Orion::AutomationParameter::Pan;
            lane.component = std::make_unique<AutomationLaneComponent>(track, Orion::AutomationParameter::Pan);
            lane.component->setPixelsPerSecond(pixelsPerSecond);
            lane.component->setSampleRate(engine.getSampleRate());
            lane.component->setInterceptsMouseClicks(laneState.activeParamType == Orion::AutomationParameter::Pan, false);
            addAndMakeVisible(lane.component.get());
            automationLanes.push_back(std::move(lane));
        }
    }
};





namespace {
    std::shared_ptr<Orion::AudioTrack> getClipTrack(Orion::AudioEngine& engine, std::shared_ptr<Orion::AudioClip> c) {
        for (auto& t : engine.getTracks()) {
            if (auto clips = t->getClips()) {
                 for (auto& cl : *clips) if (cl == c) return t;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Orion::AudioTrack> getMidiClipTrack(Orion::AudioEngine& engine, std::shared_ptr<Orion::MidiClip> c) {
        for (auto& t : engine.getTracks()) {
            if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t)) {
                if (auto clips = inst->getMidiClips()) {
                     for (auto& cl : *clips) if (cl == c) return t;
                }
            }
        }
        return nullptr;
    }
}


TrackListComponent::TrackListComponent(Orion::AudioEngine& e, Orion::CommandHistory& history)
    : engine(e), commandHistory(history)
{
}
TrackListComponent::~TrackListComponent() {}

int TrackListComponent::getTrackHeightAtIndex(int index) const
{
    auto tracks = engine.getTracks();
    if (index < 0 || index >= (int)tracks.size()) return trackHeight;
    return trackHeight + getAutomationExtraHeight(tracks[(size_t)index]);
}

int TrackListComponent::getTrackIndexForTrack(const std::shared_ptr<Orion::AudioTrack>& track) const
{
    if (!track) return -1;
    auto tracks = engine.getTracks();
    for (int i = 0; i < (int)tracks.size(); ++i) {
        if (tracks[(size_t)i] == track) return i;
    }
    return -1;
}

int TrackListComponent::getDropIndexForY(int y) const
{
    auto tracks = engine.getTracks();
    if (tracks.empty()) return -1;

    int cursorY = 0;
    for (int i = 0; i < (int)tracks.size(); ++i) {
        int h = getTrackHeightAtIndex(i);
        int mid = cursorY + h / 2;
        if (y < mid) return i;
        cursorY += h;
    }
    return (int)tracks.size() - 1;
}

int TrackListComponent::getTrackTopForIndex(int index) const
{
    if (index <= 0) return 0;
    auto tracks = engine.getTracks();
    int clamped = juce::jlimit(0, (int)tracks.size(), index);
    int y = 0;
    for (int i = 0; i < clamped; ++i) {
        y += getTrackHeightAtIndex(i);
    }
    return y;
}

void TrackListComponent::clearTrackReorderPreview()
{
    isReorderingTrack = false;
    reorderTrack.reset();
    reorderTargetIndex = -1;
    repaint();
}

void TrackListComponent::updateHeaders()
{
    headers.clear();
    for (auto& h : headers) removeChildComponent(h.get());
    removeAllChildren();

    auto tracks = engine.getTracks();
    int y = 0;
    int h = trackHeight;

    for (auto& track : tracks) {
        auto header = std::make_unique<TrackHeaderComponent>(engine, track, commandHistory);

        header->onSelected = [this, track] {
            if (onTrackSelected) onTrackSelected(track);
        };

        header->onDeleted = [this, track] {
            if (onTrackDeleted) onTrackDeleted(track);
        };

        header->onDuplicate = [this, track] {
            duplicateTrack(track);
        };

        header->onSelectAll = [this, track] {
            if (onTrackSelectAllClips) onTrackSelectAllClips(track);
        };

        header->onHeightChanged = [this] {
            updateHeaders();
            if (onLayoutChanged) onLayoutChanged();
        };

        header->onDragStarted = [this, track](juce::Point<int> screenPos) {
            isReorderingTrack = true;
            reorderTrack = track;
            reorderTargetIndex = getDropIndexForY(getLocalPoint(nullptr, screenPos).y);
            repaint();
        };

        header->onDragMoved = [this](juce::Point<int> screenPos) {
            if (!isReorderingTrack || !reorderTrack) return;
            reorderTargetIndex = getDropIndexForY(getLocalPoint(nullptr, screenPos).y);
            repaint();
        };

        header->onDragEnded = [this](juce::Point<int> screenPos) {
            if (!isReorderingTrack || !reorderTrack) {
                clearTrackReorderPreview();
                return;
            }

            reorderTargetIndex = getDropIndexForY(getLocalPoint(nullptr, screenPos).y);
            if (reorderTargetIndex >= 0 && onTrackDragToIndex) {
                onTrackDragToIndex(reorderTrack, reorderTargetIndex);
            }

            clearTrackReorderPreview();
        };

        int currentH = h;
        currentH += getAutomationExtraHeight(track);

        addAndMakeVisible(header.get());
        header->setBounds(0, y, getWidth(), currentH);
        headers.push_back(std::move(header));
        y += currentH;
    }
    setSize(getWidth(), std::max(getParentHeight(), y));
}

void TrackListComponent::setSelectedTrack(std::shared_ptr<Orion::AudioTrack> track)
{
    for (auto& h : headers)
        if (h) h->setSelected(h->getTrack() == track);
}

void TrackListComponent::setTrackHeight(int h)
{
    trackHeight = h;
    resized();
}

void TrackListComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isAltDown()) {
        int newHeight = trackHeight;
        if (wheel.deltaY > 0) newHeight += 10;
        else newHeight -= 10;
        newHeight = juce::jlimit(60, 400, newHeight);

        if (newHeight != trackHeight) {
            setTrackHeight(newHeight);
            if (onTrackHeightChanged) onTrackHeightChanged(newHeight);
        }
    } else {
        if (auto* p = getParentComponent()) p->mouseWheelMove(e, wheel);
    }
}

void TrackListComponent::duplicateTrack(std::shared_ptr<Orion::AudioTrack> src)
{
    if (!src) return;

    std::shared_ptr<Orion::AudioTrack> dest;


    if (auto* inst = dynamic_cast<Orion::InstrumentTrack*>(src.get()))
    {
         dest = engine.createInstrumentTrack();
         auto* destInst = dynamic_cast<Orion::InstrumentTrack*>(dest.get());


         auto clips = inst->getMidiClips();
         if (clips) {
             for (const auto& c : *clips) {
                 auto newClip = c->clone();
                 destInst->addMidiClip(newClip);
             }
         }
    }
    else
    {
         dest = engine.createTrack();


         auto clips = src->getClips();
         if (clips) {
             for (const auto& c : *clips) {
                 auto newClip = std::make_shared<Orion::AudioClip>();
                 newClip->setStartFrame(c->getStartFrame());
                 newClip->setLengthFrames(c->getLengthFrames());
                 newClip->setSourceStartFrame(c->getSourceStartFrame());
                 newClip->setName(c->getName());
                 newClip->setGain(c->getGain());
                 newClip->setPitch(c->getPitch());
                 const float* col = c->getColor();
                 newClip->setColor(col[0], col[1], col[2]);
                 newClip->setFile(c->getFile());

                 dest->addClip(newClip);
             }
         }
    }

    if (dest) {
        dest->setName(src->getName() + " Copy");
        dest->setVolume(src->getVolume());
        dest->setPan(src->getPan());
        dest->setMute(src->isMuted());
        dest->setSolo(src->isSolo());
        const float* color = src->getColor();
        dest->setColor(color[0], color[1], color[2]);
    }
}

void TrackListComponent::paint(juce::Graphics& g) {
    g.fillAll(findColour(OrionLookAndFeel::trackHeaderPanelColourId));

    if (isReorderingTrack && reorderTargetIndex >= 0) {
        int targetTop = getTrackTopForIndex(reorderTargetIndex);
        int targetHeight = getTrackHeightAtIndex(reorderTargetIndex);
        g.setColour(juce::Colours::cyan.withAlpha(0.12f));
        g.fillRect(0, targetTop, getWidth(), targetHeight);
        g.setColour(juce::Colours::cyan.withAlpha(0.9f));
        g.drawRect(0, targetTop, getWidth(), targetHeight, 2);
    }
}

void TrackListComponent::resized() {
    int y = 0;
    for (auto& header : headers) {
        int h = trackHeight;
        if (auto t = header->getTrack()) {
             h += getAutomationExtraHeight(t);
        }
        header->setBounds(0, y, getWidth(), h);
        y += h;
    }
}


TimelineContentComponent::TimelineContentComponent(Orion::AudioEngine& e) : engine(e)
{
    setWantsKeyboardFocus(true);
}
TimelineContentComponent::~TimelineContentComponent() {}

void TimelineContentComponent::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    grabKeyboardFocus();
}

void TimelineContentComponent::updateLanes()
{
    lanes.clear();
    removeAllChildren();

    auto tracks = engine.getTracks();
    int y = 0;
    int h = trackHeight;

    for (auto& track : tracks) {
        auto lane = std::make_unique<TimelineLaneComponent>(track, engine, pixelsPerSecond, snapFunc, toolGetter);
        lane->onClipSelected = [this](std::shared_ptr<Orion::AudioTrack> t, std::shared_ptr<Orion::AudioClip> c, bool mod) {
            juce::ignoreUnused(t);
            if (onClipSelected) onClipSelected(c, mod);
        };
        lane->onMidiClipSelected = [this](std::shared_ptr<Orion::AudioTrack> t, std::shared_ptr<Orion::MidiClip> c, bool mod) {
            if (onMidiClipSelected) {
                if (auto instTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t)) {
                    onMidiClipSelected(c, instTrack, mod);
                }
            }
        };
        lane->onMidiClipOpened = [this](std::shared_ptr<Orion::AudioTrack> t, std::shared_ptr<Orion::MidiClip> c) {
            if (onMidiClipOpened) {
                if (auto instTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t)) {
                    onMidiClipOpened(c, instTrack);
                }
            }
        };

        lane->onBackgroundMouseDown = [this](const juce::MouseEvent& e) {
            if (onBackgroundMouseDown) onBackgroundMouseDown(e.getEventRelativeTo(this));
        };
        lane->onBackgroundMouseDrag = [this](const juce::MouseEvent& e) {
            if (onBackgroundMouseDrag) onBackgroundMouseDrag(e.getEventRelativeTo(this));
        };
        lane->onBackgroundMouseUp = [this](const juce::MouseEvent& e) {
            if (onBackgroundMouseUp) onBackgroundMouseUp(e.getEventRelativeTo(this));
        };

        int currentH = h;
        currentH += getAutomationExtraHeight(track);

        addAndMakeVisible(lane.get());
        lane->setBounds(0, y, getWidth(), currentH);
        lanes.push_back(std::move(lane));
        y += currentH;
    }


    double maxEndFrame = 0;
    for (auto& t : tracks) {
        auto clips = t->getClips();
        if (clips) {
            for (auto& c : *clips) {
                double end = (double)(c->getStartFrame() + c->getLengthFrames());
                if (end > maxEndFrame) maxEndFrame = end;
            }
        }
        if (auto* inst = dynamic_cast<Orion::InstrumentTrack*>(t.get())) {
            auto mclips = inst->getMidiClips();
            if (mclips) {
                for (auto& c : *mclips) {
                    double end = (double)(c->getStartFrame() + c->getLengthFrames());
                    if (end > maxEndFrame) maxEndFrame = end;
                }
            }
        }
    }
    double sr = engine.getSampleRate();
    if (sr <= 0) sr = 44100.0;
    double duration = maxEndFrame / sr;
    if (duration < 300.0) duration = 300.0;

    setSize((int)(duration * pixelsPerSecond), std::max(getParentHeight(), y));
}

void TimelineContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::timelineBackgroundColourId));


    g.setColour(findColour(OrionLookAndFeel::timelineGridColourId).withAlpha(0.6f));

    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) sampleRate = 44100.0;

    auto tempos = engine.getTempoMarkers();
    auto sigs = engine.getTimeSigMarkers();

    uint64_t currentFrame = 0;
    double currentX = 0;

    float currentBpm = engine.getBpm();
    int currentNum = 4;
    int currentDen = 4;
    engine.getTimeSignature(currentNum, currentDen);

    size_t tIdx = 0;
    size_t sIdx = 0;

    auto clip = g.getClipBounds();
    double visibleLeft = clip.getX();
    double visibleRight = clip.getRight();


    while (tIdx < tempos.size() && tempos[tIdx].position <= 0) {
        currentBpm = tempos[tIdx].bpm;
        tIdx++;
    }
    while (sIdx < sigs.size() && sigs[sIdx].position <= 0) {
        currentNum = sigs[sIdx].numerator;
        currentDen = sigs[sIdx].denominator;
        sIdx++;
    }

    int barIndex = 1;
    int safety = 0;
    while (currentX < visibleRight + 100 && safety++ < 20000)
    {

        while (tIdx < tempos.size() && tempos[tIdx].position <= currentFrame) {
            currentBpm = tempos[tIdx].bpm;
            tIdx++;
        }
        while (sIdx < sigs.size() && sigs[sIdx].position <= currentFrame) {
            currentNum = sigs[sIdx].numerator;
            currentDen = sigs[sIdx].denominator;
            sIdx++;
        }

        double quartersPerBar = (double)currentNum * (4.0 / (double)currentDen);
        double secondsPerBar = quartersPerBar * (60.0 / currentBpm);
        double barWidth = secondsPerBar * pixelsPerSecond;
        uint64_t framesPerBar = (uint64_t)(secondsPerBar * sampleRate);
        if (framesPerBar == 0) framesPerBar = 1;


        int drawInterval = 1;
        if (barWidth < 10.0) drawInterval = 16;
        else if (barWidth < 20.0) drawInterval = 8;
        else if (barWidth < 40.0) drawInterval = 4;
        else if (barWidth < 80.0) drawInterval = 2;

        bool shouldDrawBar = ((barIndex - 1) % drawInterval == 0);

        if (currentX + barWidth >= visibleLeft) {
            if (shouldDrawBar) {

                g.setColour(findColour(OrionLookAndFeel::timelineGridColourId));
                g.drawVerticalLine((int)currentX, 0.0f, (float)getHeight());
            }


            if (barWidth > 20) {
                g.setColour(findColour(OrionLookAndFeel::timelineGridColourId).withAlpha(0.3f));
                double pixelsPerBeat = barWidth / (double)currentNum;
                for (int b = 1; b < currentNum; ++b) {
                    float bx = (float)(currentX + b * pixelsPerBeat);
                    if (bx > visibleRight) break;
                    if (bx >= visibleLeft) {
                        g.drawVerticalLine((int)bx, 0.0f, (float)getHeight());
                    }
                }
            }
        }

        currentX += barWidth;
        currentFrame += framesPerBar;
        barIndex++;
    }


}

void TimelineContentComponent::paintOverChildren(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void TimelineContentComponent::setPixelsPerSecond(double pps)
{
    pixelsPerSecond = pps;
    for (auto& lane : lanes)
        lane->setPixelsPerSecond(pps);


    double maxEndFrame = 0;
    auto tracks = engine.getTracks();
    for (auto& t : tracks) {
        auto clips = t->getClips();
        if (clips) {
            for (auto& c : *clips) {
                double end = (double)(c->getStartFrame() + c->getLengthFrames());
                if (end > maxEndFrame) maxEndFrame = end;
            }
        }
        if (auto* inst = dynamic_cast<Orion::InstrumentTrack*>(t.get())) {
            auto mclips = inst->getMidiClips();
            if (mclips) {
                for (auto& c : *mclips) {
                    double end = (double)(c->getStartFrame() + c->getLengthFrames());
                    if (end > maxEndFrame) maxEndFrame = end;
                }
            }
        }
    }
    double sr = engine.getSampleRate();
    if (sr <= 0) sr = 44100.0;
    double duration = maxEndFrame / sr;
    if (duration < 300.0) duration = 300.0;

    setSize((int)(duration * pixelsPerSecond), getHeight());
    repaint();
}

void TimelineContentComponent::setTrackHeight(int h)
{
    trackHeight = h;
    resized();
}

void TimelineContentComponent::setSelectionState(const std::function<bool(std::shared_ptr<Orion::AudioClip>)>& isSelectedAudio,
                                                 const std::function<bool(std::shared_ptr<Orion::MidiClip>)>& isSelectedMidi)
{
    for (auto& lane : lanes) {
        lane->setSelectionState(isSelectedAudio, isSelectedMidi);
    }
}

void TimelineContentComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isAltDown())
    {
        int newHeight = trackHeight;
        if (wheel.deltaY > 0)
            newHeight += 10;
        else
            newHeight -= 10;

        newHeight = juce::jlimit(60, 400, newHeight);

        if (newHeight != trackHeight)
        {
            setTrackHeight(newHeight);
            if (onTrackHeightChanged)
                onTrackHeightChanged(newHeight);
        }
        return;
    }

    if (event.mods.isCommandDown())
    {
        double newPps = pixelsPerSecond;
        if (wheel.deltaY > 0)
            newPps *= 1.1;
        else
            newPps /= 1.1;

        newPps = juce::jlimit(10.0, 2000.0, newPps);
        setPixelsPerSecond(newPps);
        return;
    }


    if (auto* parent = getParentComponent())
        parent->mouseWheelMove(event, wheel);
}

void TimelineContentComponent::resized()
{
    int y = 0;
    for (auto& lane : lanes) {
        int h = trackHeight;
        if (auto t = lane->getTrack()) {
             h += getAutomationExtraHeight(t);
        }
        lane->setBounds(0, y, getWidth(), h);
        y += h;
    }
}



TimelineComponent::TimelineComponent(Orion::AudioEngine& e, Orion::CommandHistory& history)
    : engine(e), commandHistory(history), trackList(e, history), timelineContent(e), ruler(e)
{

    addAndMakeVisible(snapLabel);
    snapLabel.setText("Snap:", juce::dontSendNotification);
    snapLabel.setJustificationType(juce::Justification::centredRight);



    addAndMakeVisible(snapComboBox);
    snapComboBox.addItem("Off", 1);
    snapComboBox.addItem("1/4", 2);
    snapComboBox.addItem("1/8", 3);
    snapComboBox.addItem("1/16", 4);
    snapComboBox.addItem("1/32", 5);
    snapComboBox.setSelectedId(4);
    snapComboBox.setTooltip("Snap grid");

    snapComboBox.onChange = [this] {
        int id = snapComboBox.getSelectedId();
        if (id == 1) snapDivision = 0;
        else if (id == 2) snapDivision = 4;
        else if (id == 3) snapDivision = 8;
        else if (id == 4) snapDivision = 16;
        else if (id == 5) snapDivision = 32;

        juce::String label = "Snap:";
        if (snapDivision == 0) label = "Snap Off";
        else label = "Snap 1/" + juce::String(snapDivision);
        snapLabel.setText(label, juce::dontSendNotification);
    };
    snapComboBox.onChange();

    timelineContent.setSnapFunction([this](uint64_t f) { return snapFrame(f); });
    timelineContent.setToolGetter([this] { return currentTool; });

    // Initialize default tool
    setTool(TimelineTool::Select);

    ruler.snapFunction = [this](uint64_t f) { return snapFrame(f); };

    ruler.onCursorMove = [this] {
        timelineContent.updatePlayhead();
    };


    addAndMakeVisible(addTrackButton);
    addTrackButton.setButtonText("+");
    addTrackButton.setTooltip(Orion::UiHints::withShortcutText("Add Track", "Cmd/Ctrl+T / Cmd/Ctrl+I"));

    // Ensure visibility in dark mode by forcing light text
    addTrackButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addTrackButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    // Use a distinct color (e.g. standard blue accent) so it stands out
    addTrackButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF007AFF));

    addTrackButton.onClick = [this] {
        juce::PopupMenu m;
        m.addItem("Add Audio Track", [this] {
            auto t = engine.createTrack();
            engine.removeTrack(t);
            commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
                t,
                [this](auto track) { engine.addTrack(track); refresh(); },
                [this](auto track) { engine.removeTrack(track); refresh(); }
            ));
        });
        m.addItem("Add Instrument Track", [this] {
            auto t = engine.createInstrumentTrack();
            engine.removeTrack(t);
            commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
                t,
                [this](auto track) { engine.addTrack(track); refresh(); },
                [this](auto track) { engine.removeTrack(track); refresh(); }
            ));
        });
        m.addItem("Add Bus / Aux Track", [this] {
            auto t = engine.createBusTrack();
            engine.removeTrack(t);
            commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
                t,
                [this](auto track) { engine.addTrack(track); refresh(); },
                [this](auto track) { engine.removeTrack(track); refresh(); }
            ));
        });
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(addTrackButton));
    };

    emptyStateOverlay = std::make_unique<TimelineEmptyStateOverlay>();
    emptyStateOverlay->onAddAudio = [this] {
        if (onInvokeCommand) {
            onInvokeCommand(Orion::CommandManager::AddAudioTrack);
            return;
        }
        auto t = engine.createTrack();
        engine.removeTrack(t);
        commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
            t,
            [this](auto track) { engine.addTrack(track); refresh(); },
            [this](auto track) { engine.removeTrack(track); refresh(); }
        ));
    };
    emptyStateOverlay->onAddInstrument = [this] {
        if (onInvokeCommand) {
            onInvokeCommand(Orion::CommandManager::AddInstrumentTrack);
            return;
        }
        auto t = engine.createInstrumentTrack();
        engine.removeTrack(t);
        commandHistory.push(std::make_unique<Orion::AddTrackCommand>(
            t,
            [this](auto track) { engine.addTrack(track); refresh(); },
            [this](auto track) { engine.removeTrack(track); refresh(); }
        ));
    };
    emptyStateOverlay->onImportAudio = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Import Audio", juce::File{}, "*.wav;*.mp3;*.mp4");
        juce::Component::SafePointer<TimelineComponent> sp(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectMultipleItems,
            [sp, chooser](const juce::FileChooser& fc) {
                if (!sp) return;
                const auto results = fc.getResults();
                if (results.isEmpty()) return;

                juce::StringArray files;
                for (const auto& f : results) {
                    files.add(f.getFullPathName());
                }

                auto vp = sp->timelineViewport.getBounds();
                int centerX = vp.getCentreX();
                int centerY = vp.getCentreY();
                sp->filesDropped(files, centerX, centerY);
            });
    };
    emptyStateOverlay->onShowGuidedStart = [this] {
        if (onShowGuidedStart) onShowGuidedStart();
    };
    addAndMakeVisible(emptyStateOverlay.get());
    emptyStateOverlay->setOverlayVisible(false);

    timelineContent.onClipSelected = [this](std::shared_ptr<Orion::AudioClip> c, bool mod) {
        if (!mod) {
            selection.clear();
            selection.add(c);
        } else {
            selection.toggle(c);
        }

        selection.track = nullptr;
        for (auto& t : engine.getTracks()) {
            auto clips = t->getClips();
            if (clips) {
                for (auto& cl : *clips) {
                    if (cl == c) {
                        selection.track = t;
                        break;
                    }
                }
            }
            if (selection.track) break;
        }

        updateSelectionState();
        if (onClipSelected) onClipSelected(c);
    };

    timelineContent.onMidiClipSelected = [this](std::shared_ptr<Orion::MidiClip> c, std::shared_ptr<Orion::InstrumentTrack> t, bool mod) {
        if (!mod) {
            selection.clear();
            selection.add(c);
        } else {
            selection.toggle(c);
        }
        selection.track = t;

        updateSelectionState();
        if (onMidiClipSelected) onMidiClipSelected(c, t, mod);
    };

    timelineContent.onMidiClipOpened = [this](std::shared_ptr<Orion::MidiClip> c, std::shared_ptr<Orion::InstrumentTrack> t) {
        if (onMidiClipOpened) onMidiClipOpened(c, t);
    };

    timelineContent.onBackgroundMouseDown = [this](const juce::MouseEvent& e) {
        if (e.mods.isMiddleButtonDown() || currentTool == TimelineTool::Zoom) {
            startPanning(e.getScreenPosition());
            return;
        }

        if (currentTool == TimelineTool::Listen) {
            beginListenAt(e);
            return;
        }

        bool mod = e.mods.isShiftDown() || e.mods.isCommandDown();
        if (!mod) {
            auto prevTrack = selection.track;
            selection.clear();
            updateSelectionState();
            if (prevTrack != nullptr && onTrackSelected) onTrackSelected(nullptr);
        }

        if (currentTool == TimelineTool::Select) {
            isRubberBandSelecting = true;
            rubberBandStart = e.position.toInt();
            rubberBandRect = juce::Rectangle<int>(rubberBandStart, rubberBandStart);
            repaint();
        }
    };

    timelineContent.onBackgroundMouseDrag = [this](const juce::MouseEvent& e) {
        if (isPanning) {
            dragPanning(e.getScreenPosition());
            return;
        }

        if (isListening) {
            updateListenAt(e);
            return;
        }

        if (isRubberBandSelecting) {
            auto pos = e.position.toInt();
            rubberBandRect = juce::Rectangle<int>(rubberBandStart, pos);
            repaint();
        }
    };

    timelineContent.onBackgroundMouseUp = [this](const juce::MouseEvent& e) {
        if (isPanning) {
            stopPanning();
            return;
        }

        juce::ignoreUnused(e);

        if (isListening) {
            endListen();
            return;
        }

        if (isRubberBandSelecting) {
            isRubberBandSelecting = false;

            double sr = engine.getSampleRate();
            if (sr <= 0) sr = 44100.0;
            double pps = timelineContent.getPixelsPerSecond();
            int h = timelineContent.getTrackHeight();

            auto tracks = engine.getTracks();
            for (size_t i = 0; i < tracks.size(); ++i) {
                auto track = tracks[i];
                int laneY = (int)i * h;

                if (auto clips = track->getClips()) {
                    for (auto& c : *clips) {
                        int x = (int)((c->getStartFrame() / sr) * pps);
                        int w = (int)((c->getLengthFrames() / sr) * pps);
                        if (w < 2) w = 2;

                        juce::Rectangle<int> bounds(x, laneY, w, h);

                        if (rubberBandRect.intersects(bounds)) {
                            selection.add(c);
                        }
                    }
                }

                if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(track)) {
                    if (auto mclips = inst->getMidiClips()) {
                        for (auto& c : *mclips) {
                            int x = (int)((c->getStartFrame() / sr) * pps);
                            int w = (int)((c->getLengthFrames() / sr) * pps);
                            if (w < 2) w = 2;

                            juce::Rectangle<int> bounds(x, laneY, w, h);

                            if (rubberBandRect.intersects(bounds)) {
                                selection.add(c);
                            }
                        }
                    }
                }
            }

            updateSelectionState();
            repaint();
        }
    };

    trackList.onTrackSelected = [this](std::shared_ptr<Orion::AudioTrack> t) {
        selection.track = t;
        selection.clear();
        updateSelectionState();
        if (onTrackSelected) onTrackSelected(t);
    };

    trackList.onTrackDeleted = [this](std::shared_ptr<Orion::AudioTrack> t) {
        commandHistory.push(std::make_unique<Orion::DeleteTrackCommand>(
            t,
            [this](auto track) {
                engine.removeTrack(track);
                refresh();
                if (selection.track == track) {
                     selection.track = nullptr;
                     if (onTrackSelected) onTrackSelected(nullptr);
                }
            },
            [this](auto track) { engine.addTrack(track); refresh(); }
        ));
    };

    trackList.onTrackSelectAllClips = [this](std::shared_ptr<Orion::AudioTrack> t) {
        if (!t) return;

        selection.clear();
        selection.track = t;

        if (auto clips = t->getClips()) {
            for (auto& c : *clips) selection.add(c);
        }

        if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t)) {
            if (auto mclips = inst->getMidiClips()) {
                for (auto& c : *mclips) selection.add(c);
            }
        }

        updateSelectionState();
        repaint();
    };

    trackList.onLayoutChanged = [this] {
        timelineContent.updateLanes();
        resized();
    };

    trackList.onTrackDragToIndex = [this](std::shared_ptr<Orion::AudioTrack> track, int toIndex) {
        const int fromIndex = engine.getTrackIndex(track);
        if (!track || fromIndex < 0 || toIndex < 0 || fromIndex == toIndex)
            return;

        commandHistory.push(std::make_unique<Orion::MoveTrackCommand>(
            engine, track, (size_t)fromIndex, (size_t)toIndex
        ));

        refresh();
        resized();
    };

    trackList.onTrackHeightChanged = [this](int h) {
        setTrackHeight(h);
    };

    addAndMakeVisible(headerViewport);
    headerViewport.setViewedComponent(&trackList, false);
    headerViewport.setScrollBarsShown(false, false);

    addAndMakeVisible(timelineViewport);
    timelineViewport.setViewedComponent(&timelineContent, false);
    timelineViewport.setScrollBarsShown(true, true);

    addAndMakeVisible(rulerViewport);
    rulerViewport.setViewedComponent(&ruler, false);
    rulerViewport.setScrollBarsShown(false, false);

    timelineViewport.positionMoved = [this](int x, int y) {
        juce::ignoreUnused(x);
        headerViewport.setViewPosition(0, y);
        rulerViewport.setViewPosition(x, 0);
    };

    headerViewport.positionMoved = [this](int x, int y) {
        juce::ignoreUnused(x);
        timelineViewport.setViewPosition(timelineViewport.getViewPositionX(), y);
    };

    timelineContent.onTrackHeightChanged = [this](int h) {
        setTrackHeight(h);
    };

    addAndMakeVisible(playheadOverlay);
    playheadOverlay.setAlwaysOnTop(true);

    setWantsKeyboardFocus(true);

    startTimerHz(60);
    trackList.updateHeaders();
    timelineContent.updateLanes();
    updateEmptyStateOverlay();
}

void TimelineComponent::refresh()
{
    trackList.updateHeaders();
    timelineContent.updateLanes();
    updateEmptyStateOverlay();
}

void TimelineComponent::lookAndFeelChanged()
{

    trackList.updateHeaders();
    timelineContent.updateLanes();
    updateEmptyStateOverlay();
    repaint();
}

TimelineComponent::~TimelineComponent() {}

bool TimelineComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        deleteSelection();
        return true;
    }

    if (key.getModifiers().isCommandDown())
    {
        if (key.getKeyCode() == 'D' || key.getKeyCode() == 'd') {
            duplicateSelection();
            return true;
        }
    }

    if (key.getKeyCode() == 'M' || key.getKeyCode() == 'm') {
        toggleMute();
        return true;
    }

    return false;
}

void TimelineComponent::duplicateSelection()
{
    if (selection.isEmpty()) return;

    auto macro = std::make_unique<Orion::MacroCommand>("Duplicate Selection");
    std::set<std::shared_ptr<Orion::AudioClip>> newClips;
    std::set<std::shared_ptr<Orion::MidiClip>> newMidiClips;


    uint64_t minStart = UINT64_MAX;
    uint64_t maxEnd = 0;

    for (auto& c : selection.clips) {
        if (c->getStartFrame() < minStart) minStart = c->getStartFrame();
        uint64_t end = c->getStartFrame() + c->getLengthFrames();
        if (end > maxEnd) maxEnd = end;
    }
    for (auto& c : selection.midiClips) {
        if (c->getStartFrame() < minStart) minStart = c->getStartFrame();
        uint64_t end = c->getStartFrame() + c->getLengthFrames();
        if (end > maxEnd) maxEnd = end;
    }

    if (maxEnd < minStart) return;

    uint64_t span = maxEnd - minStart;


    if (snapDivision > 0) {
         double bpm = getBpmAtFrame(engine, minStart);
         double sr = engine.getSampleRate();
         if (sr <= 0) sr = 44100.0;
         double samplesPerBeat = (60.0 / bpm) * sr;
         double gridFrames = samplesPerBeat * (4.0 / (double)snapDivision);

         double spans = (double)span / gridFrames;
         span = (uint64_t)(std::ceil(spans) * gridFrames);
         if (span == 0) span = (uint64_t)gridFrames;
    }


    for (auto& clip : selection.clips) {
        bool processed = false;
        for (auto& t : engine.getTracks()) {
            if (auto clips = t->getClips()) {
                bool found = false;
                for (auto& c : *clips) if (c == clip) { found = true; break; }
                if (found) {
                    auto newClip = clip->clone();
                    newClip->setStartFrame(clip->getStartFrame() + span);
                    macro->addCommand(std::make_unique<Orion::AddClipCommand>(newClip, t));
                    newClips.insert(newClip);
                    processed = true;
                    break;
                }
            }
        }
        if (!processed) {  }
    }


    for (auto& clip : selection.midiClips) {
        bool processed = false;
        for (auto& t : engine.getTracks()) {
            if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t)) {
                if (auto clips = inst->getMidiClips()) {
                    bool found = false;
                    for (auto& c : *clips) if (c == clip) { found = true; break; }
                    if (found) {
                        auto newClip = clip->clone();
                        newClip->setStartFrame(clip->getStartFrame() + span);
                        macro->addCommand(std::make_unique<Orion::AddMidiClipCommand>(newClip, inst));
                        newMidiClips.insert(newClip);
                        processed = true;
                        break;
                    }
                }
            }
        }
        if (!processed) {  }
    }

    pushCommand(std::move(macro));

    selection.clips = newClips;
    selection.midiClips = newMidiClips;
    timelineContent.updateLanes();
    updateSelectionState();
    repaint();
}

void TimelineComponent::toggleMute()
{
    if (selection.track) {
        selection.track->setMute(!selection.track->isMuted());
        trackList.updateHeaders();
    }
}

void TimelineComponent::setTrackHeight(int h)
{
    if (h != trackHeight)
    {
        trackHeight = h;
        timelineContent.setTrackHeight(h);
        trackList.setTrackHeight(h);
    }
}

void TimelineComponent::startPanning(juce::Point<int> screenPos) {
    isPanning = true;
    lastPanPos = screenPos;
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void TimelineComponent::dragPanning(juce::Point<int> screenPos) {
    if (isPanning) {
        auto delta = screenPos - lastPanPos;
        lastPanPos = screenPos;

        auto pos = timelineViewport.getViewPosition();
        timelineViewport.setViewPosition(pos.x - delta.x, pos.y - delta.y);
    }
}

void TimelineComponent::stopPanning() {
    isPanning = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void TimelineComponent::pushCommand(std::unique_ptr<Orion::Command> cmd)
{
    commandHistory.push(std::move(cmd));
}

void TimelineComponent::setTool(TimelineTool t)
{
    currentTool = t;
    repaint();
}

void TimelineComponent::setSnapDivision(int div)
{
    snapDivision = div;
    int id = 1;
    if (div == 4) id = 2;
    else if (div == 8) id = 3;
    else if (div == 16) id = 4;
    else if (div == 32) id = 5;
    snapComboBox.setSelectedId(id, juce::dontSendNotification);
}

bool TimelineComponent::hasAnyMidiClip() const
{
    for (const auto& t : engine.getTracks())
    {
        auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t);
        if (!inst) continue;
        if (auto clips = inst->getMidiClips())
        {
            if (!clips->empty())
                return true;
        }
    }
    return false;
}

bool TimelineComponent::createStarterMidiClip()
{
    std::shared_ptr<Orion::InstrumentTrack> instTrack;
    for (auto& t : engine.getTracks())
    {
        instTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t);
        if (instTrack) break;
    }

    if (!instTrack)
        instTrack = engine.createInstrumentTrack();

    if (!instTrack)
        return false;

    auto clip = std::make_shared<Orion::MidiClip>();
    clip->setStartFrame(0);
    clip->setLengthFrames((uint64_t)(engine.getSampleRate() * 4.0));
    clip->setName("MIDI Clip");

    instTrack->addMidiClip(clip);
    refresh();

    if (onMidiClipOpened)
        onMidiClipOpened(clip, instTrack);

    return true;
}

void TimelineComponent::deleteSelection()
{
    if (selection.isEmpty()) return;

    auto macro = std::make_unique<Orion::MacroCommand>("Delete Selection");

    for (auto& clip : selection.clips) {
        for (auto& t : engine.getTracks()) {
            if (auto clips = t->getClips()) {
                bool found = false;
                for (auto& c : *clips) if (c == clip) { found = true; break; }
                if (found) {
                    macro->addCommand(std::make_unique<Orion::RemoveClipCommand>(clip, t));
                    break;
                }
            }
        }
    }
    for (auto& clip : selection.midiClips) {
        for (auto& t : engine.getTracks()) {
            if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(t)) {
                if (auto clips = inst->getMidiClips()) {
                    bool found = false;
                    for (auto& c : *clips) if (c == clip) { found = true; break; }
                    if (found) {
                        macro->addCommand(std::make_unique<Orion::RemoveMidiClipCommand>(clip, inst));
                        break;
                    }
                }
            }
        }
    }

    pushCommand(std::move(macro));

    selection.clear();
    timelineContent.updateLanes();
    updateSelectionState();
}

void TimelineComponent::paint(juce::Graphics& g)
{

    g.fillAll(findColour(OrionLookAndFeel::mixerChassisColourId));


    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.drawLine(220.0f, 0.0f, 220.0f, (float)getHeight(), 1.0f);


    g.setColour(juce::Colours::black.withAlpha(0.05f));
    g.fillRect(220, 0, 4, getHeight());


    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.fillRect(0, 29, getWidth(), 1);


    g.setColour(findColour(OrionLookAndFeel::timelineRulerBackgroundColourId));
    g.fillRect(0, 0, 220, 30);
}

uint64_t TimelineComponent::snapFrame(uint64_t frame)
{
    if (snapDivision <= 0) return frame;

    double bpm = getBpmAtFrame(engine, frame);

    double sr = engine.getSampleRate();
    if (sr <= 0) sr = 44100.0;

    double secondsPerBeat = 60.0 / bpm;
    double samplesPerBeat = secondsPerBeat * sr;
    double samplesPerSnap = samplesPerBeat * (4.0 / (double)snapDivision);

    if (samplesPerSnap < 1.0) return frame;

    double snapped = std::round((double)frame / samplesPerSnap) * samplesPerSnap;
    return (uint64_t)snapped;
}

void TimelineComponent::updateSelectionState()
{
    timelineContent.setSelectionState(
        [this](std::shared_ptr<Orion::AudioClip> c) { return selection.contains(c); },
        [this](std::shared_ptr<Orion::MidiClip> c) { return selection.contains(c); }
    );

    trackList.setSelectedTrack(selection.track);
}

bool TimelineComponent::hasAnyClip() const
{
    auto tracks = engine.getTracks();
    for (const auto& track : tracks) {
        if (!track) continue;
        if (auto clips = track->getClips()) {
            if (!clips->empty()) return true;
        }
        if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(track)) {
            if (auto mclips = inst->getMidiClips()) {
                if (!mclips->empty()) return true;
            }
        }
    }
    return false;
}

void TimelineComponent::updateEmptyStateOverlay()
{
    if (!emptyStateOverlay) return;

    const bool showGuidedStart = Orion::SettingsManager::get().hintPolicy.quickStartEnabled;
    emptyStateOverlay->setGuidedStartVisible(showGuidedStart);

    const bool showOverlay = engine.getTracks().empty();
    emptyStateOverlay->setOverlayVisible(showOverlay);
    if (showOverlay) {
        emptyStateOverlay->toFront(false);
    }
}

void TimelineComponent::setSelectedTrack(std::shared_ptr<Orion::AudioTrack> track)
{
    if (selection.track == track)
        return;

    selection.clear();
    selection.track = track;
    updateSelectionState();
    repaint();
}

void TimelineComponent::resized()
{
    auto area = getLocalBounds();


    auto topArea = area.removeFromTop(30);


    auto topLeft = topArea.removeFromLeft(220);

    // Snap settings on the left
    snapLabel.setBounds(topLeft.removeFromLeft(50).reduced(2));
    snapComboBox.setBounds(topLeft.removeFromLeft(80).reduced(2));

    // Add Track button on the far right of this section
    addTrackButton.setBounds(topLeft.removeFromRight(32).reduced(2));

    rulerViewport.setBounds(topArea);

    auto contentArea = area;

    auto leftColumn = area.removeFromLeft(220);
    headerViewport.setBounds(leftColumn);


    timelineViewport.setBounds(area);

    trackList.setSize(220, trackList.getHeight());
    ruler.setSize(timelineContent.getWidth(), 30);

    playheadOverlay.setBounds(timelineViewport.getX(), 0, timelineViewport.getWidth(), getHeight());
    playheadOverlay.toFront(false);

    if (emptyStateOverlay) {
        emptyStateOverlay->setBounds(contentArea);
        emptyStateOverlay->toFront(false);
    }
}

void TimelineComponent::timerCallback()
{

    playheadOverlay.setParams(timelineContent.getPixelsPerSecond(), timelineViewport.getViewPositionX());
    playheadOverlay.repaint();


    ruler.repaint();


    static size_t lastTrackCount = 0;
    if (engine.getTracks().size() != lastTrackCount) {
        lastTrackCount = engine.getTracks().size();
        trackList.updateHeaders();
        timelineContent.updateLanes();
    }

    if (ruler.getWidth() != timelineContent.getWidth()) {
        ruler.setSize(timelineContent.getWidth(), 30);
    }
    ruler.setPixelsPerSecond(timelineContent.getPixelsPerSecond());

    updateEmptyStateOverlay();
}


bool TimelineComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files) {
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3") || f.endsWithIgnoreCase(".mp4"))
            return true;
    }
    return false;
}

void TimelineComponent::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(files);
    isDragging = true;
    dragX = x;
    dragY = y;
    repaint();
}

void TimelineComponent::fileDragMove(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(files);
    dragX = x;
    dragY = y;
    repaint();
}

void TimelineComponent::fileDragExit(const juce::StringArray& files)
{
    juce::ignoreUnused(files);
    isDragging = false;
    repaint();
}

void TimelineComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    isDragging = false;
    repaint();

    int contentX = x - timelineViewport.getX() + timelineViewport.getViewPositionX();
    int contentY = y - timelineViewport.getY() + timelineViewport.getViewPositionY();

    if (contentX < 0) contentX = 0;

    double pps = timelineContent.getPixelsPerSecond();
    if (pps <= 0.1) pps = 50.0;

    double time = contentX / pps;
    int h = trackHeight;
    if (h <= 0) h = 100;
    int trackIndex = contentY / h;

    for (auto& file : files)
    {
        std::shared_ptr<Orion::AudioTrack> targetTrack;
        int currentTrackCount = (int)engine.getTracks().size();

        if (trackIndex >= 0 && trackIndex < currentTrackCount)
        {
            targetTrack = engine.getTracks()[trackIndex];
        }
        else
        {
            targetTrack = engine.createTrack();
            trackIndex = currentTrackCount + 1;
        }

        if (targetTrack)
        {
            auto clip = std::make_shared<Orion::AudioClip>();
            if (clip->loadFromFile(file.toStdString(), (uint32_t)engine.getSampleRate()))
            {
                 clip->setStartFrame((uint64_t)(time * engine.getSampleRate()));
                 targetTrack->addClip(clip);
            }
        }
    }

    trackList.updateHeaders();
    timelineContent.updateLanes();
}

void TimelineComponent::paintOverChildren(juce::Graphics& g)
{
    if ((!emptyStateOverlay || !emptyStateOverlay->isVisible())
        && engine.getTracks().empty()
        && Orion::SettingsManager::get().hintPolicy.contextualHintsEnabled)
    {
        auto r = timelineViewport.getBounds().reduced(16);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawText("Drop audio or click + to add your first track", r.removeFromTop(26), juce::Justification::centred, true);
        g.setColour(juce::Colours::white.withAlpha(0.28f));
        g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
        g.drawText("Tip: Cmd/Ctrl+T adds audio, Cmd/Ctrl+I adds instrument", r.removeFromTop(22), juce::Justification::centred, true);
    }

    if (isRubberBandSelecting)
    {
        int vx = timelineViewport.getX();
        int vy = timelineViewport.getY();
        int vpx = timelineViewport.getViewPositionX();
        int vpy = timelineViewport.getViewPositionY();

        auto screenRect = rubberBandRect.translated(vx - vpx, vy - vpy);

        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(screenRect);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(screenRect, 1);
    }

    if (isDragging)
    {
        int vx = timelineViewport.getX();
        int vw = timelineViewport.getWidth();
        int vy = timelineViewport.getY();

        g.saveState();
        g.reduceClipRegion(vx, vy, vw, timelineViewport.getHeight());

        int h = trackHeight;
        if (h <= 0) h = 100;
        int trackIndex = dragTargetTrackIndex;
        if (trackIndex == -1) {
             int contentY = dragY - vy + timelineViewport.getViewPositionY();
             trackIndex = contentY / h;
        }
        int screenY = vy + (trackIndex * h) - timelineViewport.getViewPositionY();


        g.setColour(findColour(OrionLookAndFeel::timelineGridColourId).withAlpha(0.5f));
        g.fillRect(vx, screenY, vw, h);
        g.setColour(juce::Colours::grey);
        g.drawRect(vx, screenY, vw, h, 1);

        if (draggedAudioClip || draggedMidiClip)
        {
            uint64_t leaderStart = 0;
            std::shared_ptr<Orion::AudioTrack> leaderTrack;
            if (draggedAudioClip) {
                leaderStart = draggedAudioClip->getStartFrame();
                leaderTrack = getClipTrack(engine, draggedAudioClip);
            } else {
                leaderStart = draggedMidiClip->getStartFrame();
                leaderTrack = getMidiClipTrack(engine, draggedMidiClip);
            }

            if (leaderTrack) {
                double pps = timelineContent.getPixelsPerSecond();
                double sr = engine.getSampleRate();
                int clipX = dragX - vx + timelineViewport.getViewPositionX();
                double time = clipX / pps;
                uint64_t newLeaderStart = (uint64_t)(time * sr);
                newLeaderStart = snapFrame(newLeaderStart);

                int64_t frameDelta = (int64_t)newLeaderStart - (int64_t)leaderStart;

                int leaderTrackIdx = -1;
                auto tracks = engine.getTracks();
                for(size_t i=0; i<tracks.size(); ++i) if (tracks[i] == leaderTrack) { leaderTrackIdx = (int)i; break; }

                int targetTrackIdx = dragTargetTrackIndex;
                if (targetTrackIdx == -1) targetTrackIdx = leaderTrackIdx;
                int trackDelta = targetTrackIdx - leaderTrackIdx;


                for (auto& clip : selection.clips) {
                     if (dragOriginals.find(clip.get()) == dragOriginals.end()) continue;
                     auto orig = dragOriginals[clip.get()];

                     int64_t newStart = (int64_t)orig.startFrame + frameDelta;
                     if (newStart < 0) newStart = 0;

                     int origTrackIdx = -1;
                     for(size_t i=0; i<tracks.size(); ++i) if (tracks[i] == orig.track) { origTrackIdx = (int)i; break; }

                     int newTrackIdx = origTrackIdx + trackDelta;

                     if (newTrackIdx >= 0) {
                         int ghostY = vy + (newTrackIdx * h) - timelineViewport.getViewPositionY();
                         int ghostX = vx + (int)((double)newStart / sr * pps) - timelineViewport.getViewPositionX();
                         int ghostW = (int)((double)clip->getLengthFrames() / sr * pps);
                         if (ghostW < 2) ghostW = 2;

                         juce::Rectangle<float> r((float)ghostX, (float)ghostY + 1, (float)ghostW, (float)h - 2);
                         const float* c = clip->getColor();
                         g.setColour(juce::Colour::fromFloatRGBA(c[0], c[1], c[2], 0.6f));
                         g.fillRoundedRectangle(r, 4.0f);
                         g.setColour(juce::Colours::white.withAlpha(0.8f));
                         g.drawRoundedRectangle(r, 4.0f, 1.0f);
                     }
                }


                for (auto& clip : selection.midiClips) {
                     if (dragOriginals.find(clip.get()) == dragOriginals.end()) continue;
                     auto orig = dragOriginals[clip.get()];

                     int64_t newStart = (int64_t)orig.startFrame + frameDelta;
                     if (newStart < 0) newStart = 0;

                     int origTrackIdx = -1;
                     for(size_t i=0; i<tracks.size(); ++i) if (tracks[i] == orig.track) { origTrackIdx = (int)i; break; }

                     int newTrackIdx = origTrackIdx + trackDelta;

                     if (newTrackIdx >= 0) {
                         int ghostY = vy + (newTrackIdx * h) - timelineViewport.getViewPositionY();
                         int ghostX = vx + (int)((double)newStart / sr * pps) - timelineViewport.getViewPositionX();
                         int ghostW = (int)((double)clip->getLengthFrames() / sr * pps);
                         if (ghostW < 2) ghostW = 2;

                         juce::Rectangle<float> r((float)ghostX, (float)ghostY + 1, (float)ghostW, (float)h - 2);
                         const float* c = clip->getColor();
                         g.setColour(juce::Colour::fromFloatRGBA(c[0], c[1], c[2], 0.6f));
                         g.fillRoundedRectangle(r, 4.0f);
                         g.setColour(juce::Colours::black);
                         g.drawRoundedRectangle(r, 4.0f, 1.0f);
                     }
                }

                const bool isCopy = juce::ModifierKeys::getCurrentModifiers().isAltDown();
                g.setColour(juce::Colours::white.withAlpha(0.9f));
                g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
                g.drawText(isCopy ? "Copy selection (Alt held)" : "Move selection (hold Alt to copy)",
                           vx + 12, vy + 6, 280, 22, juce::Justification::left, false);
            }
        }
        else
        {

            g.setColour(juce::Colours::grey);
            if (dragX >= vx && dragX < vx + vw)
                g.drawLine((float)dragX, (float)vy, (float)dragX, (float)getHeight(), 2.0f);

            g.setColour(findColour(juce::Label::textColourId));
             if (trackIndex >= (int)engine.getTracks().size()) {
                  g.drawText("+ New Track", vx + 10, screenY + 10, 200, 20, juce::Justification::left);
             } else {
                  g.drawText("Add to Track", vx + 10, screenY + 10, 200, 20, juce::Justification::left);
             }
        }

        g.restoreState();
    }
}


bool TimelineComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "AudioFile") return true;
    if (dragSourceDetails.description == "AudioClip") return true;
    if (dragSourceDetails.description == "MidiClip") return true;
    if (dragSourceDetails.description.toString().startsWith("Plugin:")) return true;
    return false;
}

void TimelineComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    isDragging = true;
    dragX = dragSourceDetails.localPosition.x;
    dragY = dragSourceDetails.localPosition.y;

    draggedAudioClip = nullptr;
    draggedMidiClip = nullptr;
    dragOffsetPixels = 0;
    dragOriginals.clear();

    int rawX = dragX;
    int contentX = rawX - timelineViewport.getX() + timelineViewport.getViewPositionX();
    juce::ignoreUnused(contentX);

    double pps = timelineContent.getPixelsPerSecond();
    if (pps <= 0.1) pps = 50.0;
    double sr = engine.getSampleRate();
    if (sr <= 0) sr = 44100.0;


    auto cacheOriginals = [this]() {
        for (auto& c : selection.clips) {
             std::shared_ptr<Orion::AudioTrack> t;
             for (auto& track : engine.getTracks()) {
                 if (auto clips = track->getClips()) {
                     for (auto& cl : *clips) if (cl == c) { t = track; break; }
                 }
                 if (t) break;
             }
             if (t) dragOriginals[c.get()] = { c->getStartFrame(), t };
        }
        for (auto& c : selection.midiClips) {
             std::shared_ptr<Orion::AudioTrack> t;
             for (auto& track : engine.getTracks()) {
                 if (auto inst = std::dynamic_pointer_cast<Orion::InstrumentTrack>(track)) {
                     if (auto clips = inst->getMidiClips()) {
                         for (auto& cl : *clips) if (cl == c) { t = track; break; }
                     }
                 }
                 if (t) break;
             }
             if (t) dragOriginals[c.get()] = { c->getStartFrame(), t };
        }
    };

    if (dragSourceDetails.description == "AudioClip")
    {
        if (auto* comp = dynamic_cast<ClipComponent*>(dragSourceDetails.sourceComponent.get()))
        {
            if (auto clip = comp->getClip())
            {
                draggedAudioClip = clip;
                int64_t startFrame = clip->getStartFrame();
                int clipX = (int)((double)startFrame / sr * pps);
                dragOffsetPixels = contentX - clipX;

                if (!selection.contains(clip)) {
                    selection.clear();
                    selection.add(clip);
                    updateSelectionState();
                }
                cacheOriginals();
            }
        }
    }
    else if (dragSourceDetails.description == "MidiClip")
    {
        if (auto* comp = dynamic_cast<MidiClipComponent*>(dragSourceDetails.sourceComponent.get()))
        {
            if (auto clip = comp->getClip())
            {
                draggedMidiClip = clip;
                int64_t startFrame = clip->getStartFrame();
                int clipX = (int)((double)startFrame / sr * pps);
                dragOffsetPixels = contentX - clipX;

                if (!selection.contains(clip)) {
                    selection.clear();
                    selection.add(clip);
                    updateSelectionState();
                }
                cacheOriginals();
            }
        }
    }

    int contentY = dragY - timelineViewport.getY() + timelineViewport.getViewPositionY();
    if (contentY < 0) contentY = 0;
    int h = trackHeight;
    if (h <= 0) h = 100;
    dragTargetTrackIndex = contentY / h;

    repaint();
}

void TimelineComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    int rawX = dragSourceDetails.localPosition.x;
    dragY = dragSourceDetails.localPosition.y;

    if (draggedAudioClip || draggedMidiClip)
    {
        int contentX = rawX - timelineViewport.getX() + timelineViewport.getViewPositionX();
        int clipStartX = contentX - dragOffsetPixels;

        double pps = timelineContent.getPixelsPerSecond();
        if (pps <= 0.1) pps = 50.0;
        double sr = engine.getSampleRate();
        if (sr <= 0) sr = 44100.0;


        double time = (double)clipStartX / pps;
        if (time < 0) time = 0;
        uint64_t frame = (uint64_t)(time * sr);
        uint64_t snappedFrame = snapFrame(frame);

        int snappedX = (int)((double)snappedFrame / sr * pps);
        dragX = snappedX - timelineViewport.getViewPositionX() + timelineViewport.getX();
    }
    else
    {
        dragX = rawX;
    }

    int contentY = dragY - timelineViewport.getY() + timelineViewport.getViewPositionY();
    if (contentY < 0) contentY = 0;
    int h = trackHeight;
    if (h <= 0) h = 100;

    int rawTrackIndex = contentY / h;

    if (dragTargetTrackIndex == -1) {
        dragTargetTrackIndex = rawTrackIndex;
    } else {
        int rawCenterY = (rawTrackIndex * h) + (h / 2);
        int diff = std::abs(contentY - rawCenterY);

        if (rawTrackIndex != dragTargetTrackIndex) {
            if (diff < (h * 0.4)) {
                dragTargetTrackIndex = rawTrackIndex;
            }
        }
    }

    repaint();
}

void TimelineComponent::itemDragExit(const SourceDetails& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
    isDragging = false;
    dragTargetTrackIndex = -1;
    draggedAudioClip = nullptr;
    draggedMidiClip = nullptr;
    dragOffsetPixels = 0;
    repaint();
}

void TimelineComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    isDragging = false;
    repaint();

    if (dragSourceDetails.description == "AudioFile")
    {
        auto* tree = dynamic_cast<juce::FileTreeComponent*>(dragSourceDetails.sourceComponent.get());
        if (tree)
        {
            auto file = tree->getSelectedFile();
            if (file.existsAsFile())
            {
                juce::StringArray files;
                files.add(file.getFullPathName());
                filesDropped(files, dragSourceDetails.localPosition.x, dragSourceDetails.localPosition.y);
            }
        }
    }
    else if (dragSourceDetails.description.toString().startsWith("Plugin:"))
    {
        juce::String desc = dragSourceDetails.description.toString();
        juce::String pluginPath = desc.fromFirstOccurrenceOf("Plugin: ", false, false).upToFirstOccurrenceOf("|", false, false);
        juce::String subID = desc.fromFirstOccurrenceOf("|", false, false);

        if (pluginPath.startsWith("internal:")) {
            Orion::PluginInfo info;
            info.name = pluginPath.fromFirstOccurrenceOf("internal:", false, false).toStdString();
            info.format = Orion::PluginFormat::Internal;
            info.type = Orion::PluginType::Effect;

            auto plugin = Orion::PluginManager::loadPlugin(info);
            if (plugin) {
                auto audioTrack = engine.createTrack();
                audioTrack->addEffect(std::dynamic_pointer_cast<Orion::EffectNode>(plugin));
                audioTrack->setName(plugin->getName());
                trackList.updateHeaders();
                timelineContent.updateLanes();
            }
            return;
        }

        auto available = Orion::PluginManager::getAvailablePlugins();
        Orion::PluginInfo info;
        bool found = false;

        for (const auto& p : available) {
            if (p.path == pluginPath.toStdString() && p.subID == subID.toStdString()) {
                info = p;
                found = true;
                break;
            }
        }

        if (!found) {
            // Try matching by path only if subID is empty or not found
            for (const auto& p : available) {
                if (p.path == pluginPath.toStdString()) {
                    info = p;
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            auto plugin = Orion::PluginManager::loadPlugin(info);
            if (plugin) {
                if (plugin->isInstrument()) {
                    auto instTrack = engine.createInstrumentTrack();
                    instTrack->setInstrument(plugin);
                    instTrack->setName(plugin->getName());
                } else {
                    auto audioTrack = engine.createTrack();
                    audioTrack->addEffect(std::dynamic_pointer_cast<Orion::EffectNode>(plugin));
                    audioTrack->setName(plugin->getName());
                }


                trackList.updateHeaders();
                timelineContent.updateLanes();
            }
        }
    }
    else if (draggedAudioClip || draggedMidiClip)
    {





        uint64_t leaderStart = 0;
        std::shared_ptr<Orion::AudioTrack> leaderTrack;

        if (draggedAudioClip) {
            leaderStart = draggedAudioClip->getStartFrame();
            leaderTrack = getClipTrack(engine, draggedAudioClip);
        } else {
            leaderStart = draggedMidiClip->getStartFrame();
            leaderTrack = getMidiClipTrack(engine, draggedMidiClip);
        }

        if (!leaderTrack) return;


        double pps = timelineContent.getPixelsPerSecond();
        double sr = engine.getSampleRate();
        int clipX = dragX - timelineViewport.getX() + timelineViewport.getViewPositionX();
        double time = clipX / pps;
        uint64_t newLeaderStart = (uint64_t)(time * sr);
        newLeaderStart = snapFrame(newLeaderStart);

        int64_t frameDelta = (int64_t)newLeaderStart - (int64_t)leaderStart;


        int leaderTrackIdx = -1;
        auto tracks = engine.getTracks();
        for(size_t i=0; i<tracks.size(); ++i) if (tracks[i] == leaderTrack) { leaderTrackIdx = (int)i; break; }

        int targetTrackIdx = dragTargetTrackIndex;
        if (targetTrackIdx == -1) targetTrackIdx = leaderTrackIdx;

        int trackDelta = targetTrackIdx - leaderTrackIdx;


        bool isCopy = juce::ModifierKeys::getCurrentModifiers().isAltDown();
        auto macro = std::make_unique<Orion::MacroCommand>(isCopy ? "Duplicate Selection" : "Move Selection");

        std::set<std::shared_ptr<Orion::AudioClip>> newSelectionClips;
        std::set<std::shared_ptr<Orion::MidiClip>> newSelectionMidiClips;
        std::set<std::shared_ptr<Orion::AudioTrack>> modifiedTracks;

        for (auto& clip : selection.clips) {
             if (dragOriginals.find(clip.get()) == dragOriginals.end()) continue;
             auto orig = dragOriginals[clip.get()];

             int64_t newStart = (int64_t)orig.startFrame + frameDelta;
             if (newStart < 0) newStart = 0;


             int origTrackIdx = -1;
             for(size_t i=0; i<tracks.size(); ++i) if (tracks[i] == orig.track) { origTrackIdx = (int)i; break; }

             int newTrackIdx = origTrackIdx + trackDelta;
             if (newTrackIdx >= 0 && newTrackIdx < (int)tracks.size()) {
                 auto targetTrack = tracks[newTrackIdx];
                 modifiedTracks.insert(targetTrack);

                 if (isCopy) {
                     auto newClip = clip->clone();
                     newClip->setStartFrame((uint64_t)newStart);
                     auto cmd = std::make_unique<Orion::AddClipCommand>(newClip, targetTrack);
                     macro->addCommand(std::move(cmd));
                     newSelectionClips.insert(newClip);
                 } else {
                     auto cmd = std::make_unique<Orion::MoveClipCommand>(
                         clip, orig.startFrame, (uint64_t)newStart, orig.track, targetTrack
                     );
                     macro->addCommand(std::move(cmd));
                 }
             }
        }

        for (auto& clip : selection.midiClips) {
             if (dragOriginals.find(clip.get()) == dragOriginals.end()) continue;
             auto orig = dragOriginals[clip.get()];

             int64_t newStart = (int64_t)orig.startFrame + frameDelta;
             if (newStart < 0) newStart = 0;

             int origTrackIdx = -1;
             for(size_t i=0; i<tracks.size(); ++i) if (tracks[i] == orig.track) { origTrackIdx = (int)i; break; }

             int newTrackIdx = origTrackIdx + trackDelta;
             if (newTrackIdx >= 0 && newTrackIdx < (int)tracks.size()) {
                 auto targetTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(tracks[newTrackIdx]);
                 auto origTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(orig.track);

                 if (targetTrack && origTrack) {
                     if (isCopy) {
                         auto newClip = clip->clone();
                         newClip->setStartFrame((uint64_t)newStart);
                         auto cmd = std::make_unique<Orion::AddMidiClipCommand>(newClip, targetTrack);
                         macro->addCommand(std::move(cmd));
                         newSelectionMidiClips.insert(newClip);
                     } else {
                         auto cmd = std::make_unique<Orion::MoveMidiClipCommand>(
                             clip, orig.startFrame, (uint64_t)newStart, origTrack, targetTrack
                         );
                         macro->addCommand(std::move(cmd));
                     }
                 }
             }
        }


        pushCommand(std::move(macro));


        for (auto& t : modifiedTracks) {
            t->resolveOverlaps();
        }

        if (isCopy) {
            selection.clips = newSelectionClips;
            selection.midiClips = newSelectionMidiClips;
        }

        trackList.updateHeaders();
        timelineContent.updateLanes();
        updateSelectionState();
        timelineContent.repaint();
    }

    dragTargetTrackIndex = -1;
}

void TimelineComponent::beginListenAt(const juce::MouseEvent& e)
{
    if (isListening) return;
    isListening = true;

    listenWasPlaying = (engine.getTransportState() == Orion::TransportState::Playing
                     || engine.getTransportState() == Orion::TransportState::Recording);

    updateListenAt(e);
    if (!listenWasPlaying) engine.play();
}

void TimelineComponent::updateListenAt(const juce::MouseEvent& e)
{
    double pps = timelineContent.getPixelsPerSecond();
    if (pps <= 0.1) return;

    double sr = engine.getSampleRate();
    if (sr <= 0) sr = 44100.0;

    int vx = timelineViewport.getX();
    int vpx = timelineViewport.getViewPositionX();
    int localX = (int)e.position.x + (vpx - vx);
    if (localX < 0) localX = 0;

    uint64_t frame = (uint64_t)((double)localX / pps * sr);
    if (snapDivision > 0 && !e.mods.isAltDown()) frame = snapFrame(frame);
    engine.setCursor(frame);
}

void TimelineComponent::endListen()
{
    if (!isListening) return;
    isListening = false;
    if (!listenWasPlaying) engine.pause();
}
