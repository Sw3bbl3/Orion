#include "InspectorComponent.h"
#include "orion/PluginManager.h"
#include "orion/InstrumentTrack.h"
#include "../OrionIcons.h"
#include "../PluginWindowManager.h"
#include "../OrionLookAndFeel.h"

class InspectorButton : public juce::Button
{
public:
    InspectorButton(const juce::String& name, const juce::Path& iconPath, juce::Colour activeColour)
        : juce::Button(name), path(iconPath), onColour(activeColour)
    {
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsMouseOver, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        bool isDown = shouldDrawButtonAsDown || getToggleState();
        bool isHover = shouldDrawButtonAsMouseOver;

        juce::Colour iconC;

        if (isDown) {
             iconC = onColour;
        } else {
             // Use Label text color (Dark Grey or White) which adapts to theme
             iconC = findColour(juce::Label::textColourId);
             if (!isHover) iconC = iconC.withAlpha(0.5f); // Dim when inactive
        }

        if (isHover && !isDown) {
             g.setColour(findColour(juce::Label::textColourId).withAlpha(0.05f));
             g.fillRoundedRectangle(bounds, 4.0f);
        }

        g.setColour(iconC);

        auto iconBounds = bounds.reduced(2.0f);
        auto t = path.getTransformToScaleToFit(iconBounds, true);
        g.fillPath(path, t);
    }

private:
    juce::Path path;
    juce::Colour onColour;
};

InspectorComponent::InspectorContent::InspectorContent(Orion::AudioEngine& e, InspectorComponent& p)
    : engine(e), parent(p)
{
    addAndMakeVisible(nameEditor);
    nameEditor.setText("No Selection", juce::dontSendNotification);
    nameEditor.setJustification(juce::Justification::centred);
    nameEditor.setFont(juce::Font(juce::FontOptions(16.0f)).withStyle(juce::Font::bold));
    nameEditor.setReadOnly(true);
    nameEditor.onReturnKey = [this] {
        if (currentTrack) currentTrack->setName(nameEditor.getText().toStdString());
        else if (currentClip) currentClip->setName(nameEditor.getText().toStdString());
    };

    auto createBtn = [this](std::unique_ptr<juce::Button>& btn, const juce::Path& icon, juce::Colour col) {
        btn = std::make_unique<InspectorButton>("btn", icon, col);
        btn->setClickingTogglesState(true);
        addAndMakeVisible(btn.get());
    };

    addAndMakeVisible(trackGroup);
    trackGroup.setText("TRACK");

    createBtn(muteButton, OrionIcons::getMuteIcon(), juce::Colours::red);
    createBtn(soloButton, OrionIcons::getSoloIcon(), juce::Colours::yellow);
    createBtn(armButton, OrionIcons::getRecordIcon(), juce::Colours::red);

    muteButton->onClick = [this] {
        if (currentTrack) currentTrack->setMute(muteButton->getToggleState());
    };
    soloButton->onClick = [this] {
        if (currentTrack) {
            currentTrack->setSolo(soloButton->getToggleState());
            engine.updateSoloState();
        }
    };
    armButton->onClick = [this] {
        if (currentTrack) currentTrack->setArmed(armButton->getToggleState());
    };


    addAndMakeVisible(volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    volumeSlider.setRange(0.0, 1.0);
    volumeSlider.setValue(0.75);

    volumeSlider.onValueChange = [this] {
        if (currentTrack) currentTrack->setVolume((float)volumeSlider.getValue());
    };


    addAndMakeVisible(panSlider);
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange(-1.0, 1.0);
    panSlider.setValue(0.0);

    panSlider.onValueChange = [this] {
        if (currentTrack) currentTrack->setPan((float)panSlider.getValue());
    };


    addAndMakeVisible(effectsLabel);
    effectsLabel.setText("INSERTS", juce::dontSendNotification);
    effectsLabel.setJustificationType(juce::Justification::centredLeft);
    effectsLabel.setFont(juce::Font(juce::FontOptions(12.0f)).withStyle(juce::Font::bold));


    addAndMakeVisible(effectsList);
    effectsList.setModel(this);
    effectsList.setOutlineThickness(0);

    addAndMakeVisible(addInsertButton);
    addInsertButton.onClick = [this] { showAddInsertMenu(); };


    addAndMakeVisible(clipAudioGroup);
    clipAudioGroup.setText("CLIP AUDIO");

    addAndMakeVisible(clipTimeGroup);
    clipTimeGroup.setText("TIME & PITCH");


    addAndMakeVisible(colorLabel);
    colorLabel.setText("Color", juce::dontSendNotification);


    addAndMakeVisible(colorSelector);
    colorSelector.addItem("Cyan", 1);
    colorSelector.addItem("Red", 2);
    colorSelector.addItem("Green", 3);
    colorSelector.addItem("Purple", 4);
    colorSelector.addItem("Orange", 5);
    colorSelector.onChange = [this] {
        if (currentClip) {
            float r=0,g=0,b=0;
            switch(colorSelector.getSelectedId()) {
                case 1: r=0.0f; g=0.7f; b=0.7f; break;
                case 2: r=0.8f; g=0.2f; b=0.2f; break;
                case 3: r=0.2f; g=0.8f; b=0.2f; break;
                case 4: r=0.6f; g=0.2f; b=0.8f; break;
                case 5: r=0.9f; g=0.5f; b=0.0f; break;
            }
            currentClip->setColor(r, g, b);
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };


    addAndMakeVisible(gainLabel);
    gainLabel.setText("Gain", juce::dontSendNotification);


    addAndMakeVisible(clipGainSlider);
    clipGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    clipGainSlider.setRange(0.0, 2.0, 0.01);
    clipGainSlider.setTextValueSuffix(" x");
    clipGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 18);
    clipGainSlider.setDoubleClickReturnValue(true, 1.0);
    clipGainSlider.onValueChange = [this] {
        if (currentClip) {
            currentClip->setGain((float)clipGainSlider.getValue());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(gainResetButton);
    gainResetButton.setTooltip("Reset Gain");
    gainResetButton.onClick = [this] {
        if (currentClip) {
            currentClip->setGain(1.0f);
            clipGainSlider.setValue(1.0, juce::dontSendNotification);
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };


    addAndMakeVisible(pitchLabel);
    pitchLabel.setText("Transpose", juce::dontSendNotification);


    addAndMakeVisible(clipPitchSlider);
    clipPitchSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    clipPitchSlider.setRange(-24.0, 24.0, 1.0);
    clipPitchSlider.setTextValueSuffix(" st");
    clipPitchSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 18);
    clipPitchSlider.setDoubleClickReturnValue(true, 0.0);
    clipPitchSlider.onValueChange = [this] {
        if (currentClip) {
            float p = (float)clipPitchSlider.getValue() + (float)clipFineTuneSlider.getValue() / 100.0f;
            currentClip->setPitch(p);
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(fineTuneLabel);
    fineTuneLabel.setText("Fine", juce::dontSendNotification);

    addAndMakeVisible(clipFineTuneSlider);
    clipFineTuneSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    clipFineTuneSlider.setRange(-100.0, 100.0, 1.0);
    clipFineTuneSlider.setTextValueSuffix(" ct");
    clipFineTuneSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 18);
    clipFineTuneSlider.setDoubleClickReturnValue(true, 0.0);
    clipFineTuneSlider.onValueChange = [this] {
        if (currentClip) {
            float p = (float)clipPitchSlider.getValue() + (float)clipFineTuneSlider.getValue() / 100.0f;
            currentClip->setPitch(p);
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(pitchResetButton);
    pitchResetButton.setTooltip("Reset Transpose");
    pitchResetButton.onClick = [this] {
        if (currentClip) {
            currentClip->setPitch(0.0f);
            clipPitchSlider.setValue(0.0, juce::dontSendNotification);
            clipFineTuneSlider.setValue(0.0, juce::dontSendNotification);
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(preservePitchToggle);
    preservePitchToggle.setButtonText("Preserve Length");
    preservePitchToggle.onClick = [this] {
        if (currentClip) {
            currentClip->setPreservePitch(preservePitchToggle.getToggleState());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(syncLabel);
    syncLabel.setText("Sync Mode", juce::dontSendNotification);

    addAndMakeVisible(syncModeSelector);
    syncModeSelector.addItem("Off", 1);
    syncModeSelector.addItem("Resample", 2);
    syncModeSelector.addItem("Stretch", 3);
    syncModeSelector.onChange = [this] {
        if (currentClip) {
            currentClip->setSyncMode((Orion::SyncMode)(syncModeSelector.getSelectedId() - 1));
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("Orig BPM", juce::dontSendNotification);

    addAndMakeVisible(originalBpmField);
    originalBpmField.setInputRestrictions(0, "0123456789.");
    originalBpmField.onReturnKey = [this] {
        if (currentClip) {
            currentClip->setOriginalBpm(originalBpmField.getText().getFloatValue());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    reverseButton = std::make_unique<juce::TextButton>("Reverse");
    reverseButton->setClickingTogglesState(true);
    addAndMakeVisible(reverseButton.get());
    reverseButton->onClick = [this] {
        if (currentClip) {
            currentClip->setReverse(reverseButton->getToggleState());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    loopButton = std::make_unique<juce::TextButton>("Loop");
    loopButton->setClickingTogglesState(true);
    addAndMakeVisible(loopButton.get());
    loopButton->onClick = [this] {
        if (currentClip) {
            currentClip->setLooping(loopButton->getToggleState());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(fadeInLabel);
    fadeInLabel.setText("Fade In", juce::dontSendNotification);
    addAndMakeVisible(fadeInSlider);
    fadeInSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    fadeInSlider.setRange(0.0, 100000.0, 1.0);
    fadeInSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 18);
    fadeInSlider.onValueChange = [this] {
        if (currentClip) {
            currentClip->setFadeIn((uint64_t)fadeInSlider.getValue());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };

    addAndMakeVisible(fadeOutLabel);
    fadeOutLabel.setText("Fade Out", juce::dontSendNotification);
    addAndMakeVisible(fadeOutSlider);
    fadeOutSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    fadeOutSlider.setRange(0.0, 100000.0, 1.0);
    fadeOutSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 18);
    fadeOutSlider.onValueChange = [this] {
        if (currentClip) {
            currentClip->setFadeOut((uint64_t)fadeOutSlider.getValue());
            if (parent.onClipChanged) parent.onClipChanged();
        }
    };


    addAndMakeVisible(startLabel); startLabel.setText("Timeline Start", juce::dontSendNotification);
    addAndMakeVisible(startVal);

    addAndMakeVisible(lenLabel); lenLabel.setText("Timeline Len", juce::dontSendNotification);
    addAndMakeVisible(lenVal);
}

InspectorComponent::InspectorContent::~InspectorContent()
{
    effectsList.setModel(nullptr);
}

void InspectorComponent::InspectorContent::updateSelection(std::shared_ptr<Orion::AudioClip> clip, std::shared_ptr<Orion::AudioTrack> track)
{
    currentClip = clip;
    currentTrack = track;

    if (clip) {
        nameEditor.setText(clip->getName(), false);
        nameEditor.setReadOnly(false);
        colorSelector.setSelectedId(1, juce::dontSendNotification);
        clipGainSlider.setValue(clip->getGain(), juce::dontSendNotification);

        float p = clip->getPitch();
        float st = std::round(p);
        float ct = (p - st) * 100.0f;
        clipPitchSlider.setValue(st, juce::dontSendNotification);
        clipFineTuneSlider.setValue(ct, juce::dontSendNotification);
        preservePitchToggle.setToggleState(clip->isPreservePitch(), juce::dontSendNotification);

        syncModeSelector.setSelectedId((int)clip->getSyncMode() + 1, juce::dontSendNotification);
        originalBpmField.setText(juce::String(clip->getOriginalBpm(), 2), false);

        reverseButton->setToggleState(clip->isReverse(), juce::dontSendNotification);
        loopButton->setToggleState(clip->isLooping(), juce::dontSendNotification);

        fadeInSlider.setValue((double)clip->getFadeIn(), juce::dontSendNotification);
        fadeOutSlider.setValue((double)clip->getFadeOut(), juce::dontSendNotification);

        startVal.setText(juce::String(clip->getStartFrame()), juce::dontSendNotification);
        lenVal.setText(juce::String(clip->getLengthFrames()), juce::dontSendNotification);
    }
    else if (track) {
        nameEditor.setText(track->getName(), false);
        nameEditor.setReadOnly(false);
        volumeSlider.setValue(track->getVolume(), juce::dontSendNotification);
        panSlider.setValue(track->getPan(), juce::dontSendNotification);

        muteButton->setToggleState(track->isMuted(), juce::dontSendNotification);
        soloButton->setToggleState(track->isSolo(), juce::dontSendNotification);

        auto fx = track->getEffects();
        if (fx) currentEffects = *fx;
        else currentEffects.clear();
        effectsList.updateContent();
    }

    resized();
}

void InspectorComponent::InspectorContent::resized()
{
    auto area = getLocalBounds().reduced(10);

    if (currentClip)
    {
        clipAudioGroup.setVisible(true);
        clipTimeGroup.setVisible(true);
        colorLabel.setVisible(true);
        colorSelector.setVisible(true);
        gainLabel.setVisible(true);
        clipGainSlider.setVisible(true);
        gainResetButton.setVisible(true);
        pitchLabel.setVisible(true);
        clipPitchSlider.setVisible(true);
        pitchResetButton.setVisible(true);
        fineTuneLabel.setVisible(true);
        clipFineTuneSlider.setVisible(true);
        preservePitchToggle.setVisible(true);
        syncLabel.setVisible(true);
        syncModeSelector.setVisible(true);
        bpmLabel.setVisible(true);
        originalBpmField.setVisible(true);
        reverseButton->setVisible(true);
        loopButton->setVisible(true);
        fadeInLabel.setVisible(true);
        fadeInSlider.setVisible(true);
        fadeOutLabel.setVisible(true);
        fadeOutSlider.setVisible(true);
        startLabel.setVisible(true);
        startVal.setVisible(true);
        lenLabel.setVisible(true);
        lenVal.setVisible(true);


        trackGroup.setVisible(false);
        nameEditor.setVisible(true);
        volumeSlider.setVisible(false);
        panSlider.setVisible(false);
        muteButton->setVisible(false);
        soloButton->setVisible(false);
        armButton->setVisible(false);
        effectsLabel.setVisible(false);
        effectsList.setVisible(false);
        addInsertButton.setVisible(false);

        nameEditor.setBounds(area.removeFromTop(30));
        area.removeFromTop(10);

        auto audioArea = area.removeFromTop(160);
        clipAudioGroup.setBounds(audioArea);
        audioArea.reduce(10, 20);

        auto row = audioArea.removeFromTop(24);
        colorLabel.setBounds(row.removeFromLeft(80));
        colorSelector.setBounds(row);

        audioArea.removeFromTop(8);
        row = audioArea.removeFromTop(24);
        gainLabel.setBounds(row.removeFromLeft(80));
        auto gainResetArea = row.removeFromRight(52);
        gainResetButton.setBounds(gainResetArea.reduced(2, 2));
        clipGainSlider.setBounds(row);

        audioArea.removeFromTop(8);
        row = audioArea.removeFromTop(24);
        fadeInLabel.setBounds(row.removeFromLeft(80));
        fadeInSlider.setBounds(row);

        audioArea.removeFromTop(4);
        row = audioArea.removeFromTop(24);
        fadeOutLabel.setBounds(row.removeFromLeft(80));
        fadeOutSlider.setBounds(row);

        area.removeFromTop(10);

        auto timeArea = area.removeFromTop(290);
        clipTimeGroup.setBounds(timeArea);
        timeArea.reduce(10, 20);

        row = timeArea.removeFromTop(24);
        syncLabel.setBounds(row.removeFromLeft(80));
        syncModeSelector.setBounds(row);

        timeArea.removeFromTop(8);
        row = timeArea.removeFromTop(24);
        bpmLabel.setBounds(row.removeFromLeft(80));
        originalBpmField.setBounds(row);

        timeArea.removeFromTop(8);
        row = timeArea.removeFromTop(24);
        pitchLabel.setBounds(row.removeFromLeft(80));
        auto pitchResetArea = row.removeFromRight(52);
        pitchResetButton.setBounds(pitchResetArea.reduced(2, 2));
        clipPitchSlider.setBounds(row);

        timeArea.removeFromTop(4);
        row = timeArea.removeFromTop(24);
        fineTuneLabel.setBounds(row.removeFromLeft(80));
        clipFineTuneSlider.setBounds(row);

        timeArea.removeFromTop(6);
        row = timeArea.removeFromTop(22);
        preservePitchToggle.setBounds(row);

        timeArea.removeFromTop(10);
        row = timeArea.removeFromTop(28);
        reverseButton->setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));
        loopButton->setBounds(row.reduced(2));

        timeArea.removeFromTop(12);
        row = timeArea.removeFromTop(20);
        startLabel.setBounds(row.removeFromLeft(100));
        startVal.setBounds(row);

        row = timeArea.removeFromTop(20);
        lenLabel.setBounds(row.removeFromLeft(100));
        lenVal.setBounds(row);
    }
    else if (currentTrack)
    {
        clipAudioGroup.setVisible(false);
        clipTimeGroup.setVisible(false);
        colorLabel.setVisible(false);
        colorSelector.setVisible(false);
        gainLabel.setVisible(false);
        clipGainSlider.setVisible(false);
        gainResetButton.setVisible(false);
        pitchLabel.setVisible(false);
        clipPitchSlider.setVisible(false);
        pitchResetButton.setVisible(false);
        fineTuneLabel.setVisible(false);
        clipFineTuneSlider.setVisible(false);
        preservePitchToggle.setVisible(false);
        syncLabel.setVisible(false);
        syncModeSelector.setVisible(false);
        bpmLabel.setVisible(false);
        originalBpmField.setVisible(false);
        reverseButton->setVisible(false);
        loopButton->setVisible(false);
        fadeInLabel.setVisible(false);
        fadeInSlider.setVisible(false);
        fadeOutLabel.setVisible(false);
        fadeOutSlider.setVisible(false);
        startLabel.setVisible(false);
        startVal.setVisible(false);
        lenLabel.setVisible(false);
        lenVal.setVisible(false);


        trackGroup.setVisible(true);
        volumeSlider.setVisible(true);
        panSlider.setVisible(true);
        muteButton->setVisible(true);
        soloButton->setVisible(true);
        armButton->setVisible(true);
        effectsLabel.setVisible(true);
        effectsList.setVisible(true);
        addInsertButton.setVisible(true);

        auto trackArea = area.removeFromTop(360);
        trackGroup.setBounds(trackArea);
        trackArea.reduce(10, 20);

        nameEditor.setBounds(trackArea.removeFromTop(30));
        trackArea.removeFromTop(5);

        auto btnRow = trackArea.removeFromTop(30);
        int btnW = 50;
        int gap = (trackArea.getWidth() - (btnW * 3)) / 4;
        muteButton->setBounds(trackArea.getX() + gap, btnRow.getY(), btnW, 24);
        soloButton->setBounds(trackArea.getX() + gap*2 + btnW, btnRow.getY(), btnW, 24);
        armButton->setBounds(trackArea.getX() + gap*3 + btnW*2, btnRow.getY(), btnW, 24);

        trackArea.removeFromTop(10);

        auto panArea = trackArea.removeFromTop(60);
        panSlider.setBounds(panArea.withSizeKeepingCentre(60, 60));

        trackArea.removeFromTop(10);

        auto volArea = trackArea.removeFromTop(180);
        volumeSlider.setBounds(volArea.withSizeKeepingCentre(60, 180));

        area.removeFromTop(20);

        auto fxHeaderRow = area.removeFromTop(20);
        effectsLabel.setBounds(fxHeaderRow.removeFromLeft(fxHeaderRow.getWidth() - 30));
        addInsertButton.setBounds(fxHeaderRow.reduced(2));
        effectsList.setBounds(area.removeFromTop(150));
    }
    else
    {
        trackGroup.setVisible(false);
        nameEditor.setVisible(true);
        nameEditor.setText("No Selection", false);
        nameEditor.setReadOnly(true);
        volumeSlider.setVisible(false);
        panSlider.setVisible(false);
        muteButton->setVisible(false);
        soloButton->setVisible(false);
        armButton->setVisible(false);
        effectsLabel.setVisible(false);
        effectsList.setVisible(false);
        addInsertButton.setVisible(false);

        clipAudioGroup.setVisible(false);
        clipTimeGroup.setVisible(false);
        colorLabel.setVisible(false);
        colorSelector.setVisible(false);
        gainLabel.setVisible(false);
        clipGainSlider.setVisible(false);
        gainResetButton.setVisible(false);
        pitchLabel.setVisible(false);
        clipPitchSlider.setVisible(false);
        pitchResetButton.setVisible(false);
        fineTuneLabel.setVisible(false);
        clipFineTuneSlider.setVisible(false);
        preservePitchToggle.setVisible(false);
        syncLabel.setVisible(false);
        syncModeSelector.setVisible(false);
        bpmLabel.setVisible(false);
        originalBpmField.setVisible(false);
        reverseButton->setVisible(false);
        loopButton->setVisible(false);
        fadeInLabel.setVisible(false);
        fadeInSlider.setVisible(false);
        fadeOutLabel.setVisible(false);
        fadeOutSlider.setVisible(false);
        startLabel.setVisible(false);
        startVal.setVisible(false);
        lenLabel.setVisible(false);
        lenVal.setVisible(false);

        nameEditor.setBounds(area.removeFromTop(30));
    }
}

int InspectorComponent::InspectorContent::getNumRows() { return (int)currentEffects.size(); }

void InspectorComponent::InspectorContent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= (int)currentEffects.size()) return;
    g.fillAll(findColour(OrionLookAndFeel::trackHeaderBackgroundColourId).darker(0.05f));
    if (rowIsSelected) g.fillAll(findColour(juce::Slider::rotarySliderFillColourId).withAlpha(0.2f));
    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.drawHorizontalLine(height - 1, 0.0f, (float)width);
    auto fx = currentEffects[rowNumber];
    bool bypassed = fx->isBypassed();
    int iconSize = 12;
    int x = width - iconSize - 10;
    int y = (height - iconSize) / 2;
    g.setColour(bypassed ? juce::Colours::grey : findColour(juce::Slider::rotarySliderFillColourId));
    g.drawEllipse((float)x, (float)y, (float)iconSize, (float)iconSize, 2.0f);
    if (!bypassed) g.fillEllipse((float)x + 3, (float)y + 3, (float)iconSize - 6, (float)iconSize - 6);
    g.setColour(bypassed ? juce::Colours::grey : findColour(juce::Label::textColourId));
    g.setFont(14.0f);
    g.drawText(fx->getName(), 10, 0, x - 15, height, juce::Justification::centredLeft, true);
}

void InspectorComponent::InspectorContent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row >= (int)currentEffects.size()) return;
    int width = effectsList.getWidth();
    int iconSize = 12;
    int x = width - iconSize - 10;
    if (e.x >= x - 5) {
        auto fx = currentEffects[row];
        fx->setBypass(!fx->isBypassed());
        effectsList.repaintRow(row);
    }
}

void InspectorComponent::InspectorContent::listBoxItemDoubleClicked(int row, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (row >= (int)currentEffects.size()) return;
    auto fx = currentEffects[row];
    PluginWindowManager::getInstance().showEditor(fx);
}

void InspectorComponent::InspectorContent::deleteKeyPressed(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < (int)currentEffects.size()) {
        if (currentTrack) {
            currentTrack->removeEffect(currentEffects[lastRowSelected]);
            auto fx = currentTrack->getEffects();
            if (fx) currentEffects = *fx; else currentEffects.clear();
            effectsList.updateContent();
        }
    }
}

void InspectorComponent::InspectorContent::showAddInsertMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("Add Insert");

    juce::PopupMenu internalMenu;
    auto addInternal = [this, &internalMenu](const juce::String& name) {
        internalMenu.addItem(name, [this, name] {
            Orion::PluginInfo info;
            info.name = name.toStdString();
            info.format = Orion::PluginFormat::Internal;
            info.type = Orion::PluginType::Effect;
            addInsertPlugin(info);
        });
    };

    addInternal("EQ3");
    addInternal("Compressor");
    addInternal("Limiter");
    addInternal("Delay");
    addInternal("Reverb");
    addInternal("Gain");
    addInternal("Flux Shaper");
    addInternal("Prism Stack");
    menu.addSubMenu("Orion Effects", internalMenu);

    juce::PopupMenu externalMenu;
    int effectCount = 0;
    for (const auto& p : Orion::PluginManager::getAvailablePlugins()) {
        if (p.format == Orion::PluginFormat::Internal) continue;
        if (p.type == Orion::PluginType::Instrument) continue;
        externalMenu.addItem(p.name, [this, p] { addInsertPlugin(p); });
        ++effectCount;
    }

    if (effectCount == 0) {
        externalMenu.addItem("(No scanned effects found)", false, false, [] {});
    }
    menu.addSubMenu("External Plugins", externalMenu);
    menu.addSeparator();
    menu.addItem("Rescan Plugins", [] { Orion::PluginManager::scanPlugins(true); });

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addInsertButton));
}

bool InspectorComponent::InspectorContent::addInsertPlugin(const Orion::PluginInfo& info)
{
    if (!currentTrack) return false;

    auto plugin = Orion::PluginManager::loadPlugin(info);
    if (!plugin) return false;

    plugin->setFormat(2, (size_t)engine.getBlockSize());
    plugin->setSampleRate(engine.getSampleRate());

    currentTrack->addEffect(plugin);

    auto fx = currentTrack->getEffects();
    if (fx) currentEffects = *fx; else currentEffects.clear();

    effectsList.updateContent();
    PluginWindowManager::getInstance().showEditor(plugin);
    return true;
}

InspectorComponent::InspectorComponent(Orion::AudioEngine& e) : engine(e)
{
    content = std::make_unique<InspectorContent>(engine, *this);
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(content.get(), false);
    viewport.setScrollBarsShown(true, false);
    startTimer(100);
}

InspectorComponent::~InspectorComponent() {}

void InspectorComponent::inspectClip(std::shared_ptr<Orion::AudioClip> clip)
{
    currentClip = clip;
    currentTrack = nullptr;
    content->updateSelection(clip, nullptr);
    resized();
}

void InspectorComponent::inspectTrack(std::shared_ptr<Orion::AudioTrack> track)
{
    currentTrack = track;
    currentClip = nullptr;
    content->updateSelection(nullptr, track);
    resized();
}

void InspectorComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::mixerChassisColourId));
    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.drawRect(getLocalBounds().removeFromRight(1));

    g.setColour(findColour(OrionLookAndFeel::trackHeaderBackgroundColourId));
    g.fillRect(0, 0, getWidth(), 40);
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawHorizontalLine(40, 0.0f, (float)getWidth());

    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(14.0f);
    g.drawText(currentClip ? "CLIP" : "TRACK", 0, 0, getWidth(), 40, juce::Justification::centred);
}

void InspectorComponent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40);
    viewport.setBounds(area);

    int contentHeight = 50;
    if (currentClip) contentHeight = 590;
    else if (currentTrack) contentHeight = 650;

    content->setBounds(0, 0, viewport.getMaximumVisibleWidth(), contentHeight);
}


bool InspectorComponent::isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (currentTrack) {
        if (dragSourceDetails.description.toString().startsWith("Plugin:"))
            return true;
    }
    return false;
}

void InspectorComponent::itemDropped(const DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (!currentTrack) return;

    juce::String desc = dragSourceDetails.description.toString();
    if (desc.startsWith("Plugin: "))
    {
        juce::String pathAndSubId = desc.substring(8);
        juce::String path = pathAndSubId.upToFirstOccurrenceOf("|", false, false);
        juce::String subId = pathAndSubId.substring(path.length() + 1);

        if (path.startsWith("internal:")) {
            Orion::PluginInfo internalInfo;
            internalInfo.name = path.fromFirstOccurrenceOf("internal:", false, false).toStdString();
            internalInfo.format = Orion::PluginFormat::Internal;
            internalInfo.type = Orion::PluginType::Effect;

            auto node = Orion::PluginManager::loadPlugin(internalInfo);
            if (!node) return;
            node->setFormat(2, (size_t)engine.getBlockSize());
            node->setSampleRate(engine.getSampleRate());
            currentTrack->addEffect(node);

            auto fx = currentTrack->getEffects();
            if (fx) content->currentEffects = *fx;
            else content->currentEffects.clear();
            content->effectsList.updateContent();
            return;
        }

        auto plugins = Orion::PluginManager::getAvailablePlugins();
        Orion::PluginInfo info;
        bool found = false;

        for (const auto& p : plugins) {
            if (p.path == path.toStdString() && p.subID == subId.toStdString()) {
                info = p;
                found = true;
                break;
            }
        }

        if (!found) {
            info.path = path.toStdString();
            info.subID = subId.toStdString();
            info.format = (subId.isNotEmpty()) ? Orion::PluginFormat::VST3 : Orion::PluginFormat::VST2;
            info.type = Orion::PluginType::Unknown;
        }

        auto node = Orion::PluginManager::loadPlugin(info);
        if (node) {
            node->setFormat(2, (size_t)engine.getBlockSize());
            node->setSampleRate(engine.getSampleRate());

            if (info.type == Orion::PluginType::Instrument || node->isInstrument()) {
                if (auto* instTrack = dynamic_cast<Orion::InstrumentTrack*>(currentTrack.get())) {
                    instTrack->setInstrument(node);
                    return;
                }
            }

            currentTrack->addEffect(node);
            auto fx = currentTrack->getEffects();
            if (fx) content->currentEffects = *fx;
            else content->currentEffects.clear();
            content->effectsList.updateContent();
        }
    }
}

void InspectorComponent::timerCallback()
{
    if (currentTrack) {
        auto fx = currentTrack->getEffects();
        if (fx && fx->size() != content->currentEffects.size()) {
            content->currentEffects = *fx;
            content->effectsList.updateContent();
        }

        if (content->muteButton) content->muteButton->setToggleState(currentTrack->isMuted(), juce::dontSendNotification);
        if (content->soloButton) content->soloButton->setToggleState(currentTrack->isSolo(), juce::dontSendNotification);
        if (content->armButton) content->armButton->setToggleState(currentTrack->isArmed(), juce::dontSendNotification);
    }
}
