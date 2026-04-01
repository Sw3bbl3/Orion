#include "AdaptiveSpaceWindow.h"
#include "../OrionLookAndFeel.h"

AdaptiveSpaceWindow::AdaptiveSpaceWindow(Orion::EnvironmentNode* node) : node(node)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Adaptive Space", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(subtitleLabel);
    subtitleLabel.setText("Professional Space and Translation Controls", juce::dontSendNotification);
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF9CB3BC));
    subtitleLabel.setFont(juce::FontOptions(12.0f));

    addAndMakeVisible(versionLabel);
    versionLabel.setText("v1.1 Beta", juce::dontSendNotification);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF7AAEB4));
    versionLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));

    addAndMakeVisible(activeToggle);
    activeToggle.setToggleState(node->isActive(), juce::dontSendNotification);
    activeToggle.onClick = [this, node] { node->setActive(activeToggle.getToggleState()); };

    addAndMakeVisible(presetsLabel);
    presetsLabel.setText("Select Space", juce::dontSendNotification);
    presetsLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    presetsLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));

    addAndMakeVisible(quickMixLabel);
    quickMixLabel.setText("Quick Mix Profiles", juce::dontSendNotification);
    quickMixLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    quickMixLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));

    auto setupBtn = [&](juce::TextButton& b, Orion::EnvironmentNode::Preset p) {
        addAndMakeVisible(b);
        b.setClickingTogglesState(true);
        b.setRadioGroupId(1001);
        presetButtons.push_back(&b);

        b.onClick = [this, node, p, &b] {
            if (b.getToggleState()) {
                node->setPreset(p);
                refreshFromNode();
            }
        };
    };

    setupBtn(studioBtn, Orion::EnvironmentNode::Preset::Studio);
    setupBtn(smallRoomBtn, Orion::EnvironmentNode::Preset::SmallRoom);
    setupBtn(hallBtn, Orion::EnvironmentNode::Preset::Hall);
    setupBtn(clubBtn, Orion::EnvironmentNode::Preset::Club);
    setupBtn(carBtn, Orion::EnvironmentNode::Preset::Car);
    setupBtn(outdoorBtn, Orion::EnvironmentNode::Preset::Outdoor);
    setupBtn(phoneBtn, Orion::EnvironmentNode::Preset::Phone);

    auto setupQuickBtn = [this](juce::TextButton& b, const juce::String& name) {
        addAndMakeVisible(b);
        b.onClick = [this, name] { applyQuickMixProfile(name); };
    };
    setupQuickBtn(vocalBtn, "Vocal");
    setupQuickBtn(drumsBtn, "Drums");
    setupQuickBtn(busBtn, "Mix Bus");
    setupQuickBtn(fxBtn, "FX Send");


    setupKnob(sizeKnob, sizeLabel, "Size");
    sizeKnob.setValue(node->getSize(), juce::dontSendNotification);
    sizeKnob.onValueChange = [this, node] { node->setSize((float)sizeKnob.getValue()); };

    setupKnob(crowdKnob, crowdLabel, "Crowd");
    crowdKnob.setValue(node->getCrowd(), juce::dontSendNotification);
    crowdKnob.onValueChange = [this, node] { node->setCrowd((float)crowdKnob.getValue()); };

    setupKnob(ambienceKnob, ambienceLabel, "Ambience");
    ambienceKnob.setValue(node->getAmbience(), juce::dontSendNotification);
    ambienceKnob.onValueChange = [this, node] { node->setAmbience((float)ambienceKnob.getValue()); };

    setupKnob(intensityKnob, intensityLabel, "Glue");
    intensityKnob.setValue(node->getIntensity(), juce::dontSendNotification);
    intensityKnob.onValueChange = [this, node] { node->setIntensity((float)intensityKnob.getValue()); };

    setupKnob(toneKnob, toneLabel, "Tone");
    toneKnob.setValue(node->getTone(), juce::dontSendNotification);
    toneKnob.onValueChange = [this, node] { node->setTone((float)toneKnob.getValue()); };

    setupKnob(widthKnob, widthLabel, "Width");
    widthKnob.setValue(node->getWidth(), juce::dontSendNotification);
    widthKnob.onValueChange = [this, node] { node->setWidth((float)widthKnob.getValue()); };

    setupKnob(gritKnob, gritLabel, "Grit");
    gritKnob.setValue(node->getGrit(), juce::dontSendNotification);
    gritKnob.onValueChange = [this, node] { node->setGrit((float)gritKnob.getValue()); };

    setupKnob(preDelayKnob, preDelayLabel, "Pre-Delay");
    preDelayKnob.setValue(node->getPreDelay(), juce::dontSendNotification);
    preDelayKnob.onValueChange = [this, node] { node->setPreDelay((float)preDelayKnob.getValue()); };

    setupKnob(trimKnob, trimLabel, "Trim");
    trimKnob.setValue(node->getOutputTrim(), juce::dontSendNotification);
    trimKnob.onValueChange = [this, node] { node->setOutputTrim((float)trimKnob.getValue()); };

    addAndMakeVisible(loudnessMatchToggle);
    loudnessMatchToggle.setToggleState(node->getAutoGain(), juce::dontSendNotification);
    loudnessMatchToggle.onClick = [this, node] { node->setAutoGain(loudnessMatchToggle.getToggleState()); };

    addAndMakeVisible(resetBtn);
    resetBtn.onClick = [this, node] {
        node->setPreset(Orion::EnvironmentNode::Preset::Studio);
        node->setIntensity(0.6f);
        node->setPreDelay(0.18f);
        node->setOutputTrim(0.5f);
        node->setAutoGain(true);
        refreshFromNode();
    };

    addAndMakeVisible(storeABtn);
    storeABtn.onClick = [this] { storeSnapshot(snapshotA); };

    addAndMakeVisible(recallABtn);
    recallABtn.onClick = [this] { loadSnapshot(snapshotA); };

    addAndMakeVisible(storeBBtn);
    storeBBtn.onClick = [this] { storeSnapshot(snapshotB); };

    addAndMakeVisible(recallBBtn);
    recallBBtn.onClick = [this] { loadSnapshot(snapshotB); };

    refreshFromNode();
    updateButtonStates();
    setSize(640, 700);
}

void AdaptiveSpaceWindow::setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    addAndMakeVisible(s);
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setRange(0.0, 1.0);



    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF40E0D0));
    s.setColour(juce::Slider::thumbColourId, juce::Colours::white);

    addAndMakeVisible(l);
    l.setText(name, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::FontOptions(14.0f));
    l.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
}

void AdaptiveSpaceWindow::refreshFromNode()
{
    if (node == nullptr) return;

    activeToggle.setToggleState(node->isActive(), juce::dontSendNotification);
    sizeKnob.setValue(node->getSize(), juce::dontSendNotification);
    crowdKnob.setValue(node->getCrowd(), juce::dontSendNotification);
    ambienceKnob.setValue(node->getAmbience(), juce::dontSendNotification);
    intensityKnob.setValue(node->getIntensity(), juce::dontSendNotification);
    toneKnob.setValue(node->getTone(), juce::dontSendNotification);
    widthKnob.setValue(node->getWidth(), juce::dontSendNotification);
    gritKnob.setValue(node->getGrit(), juce::dontSendNotification);
    preDelayKnob.setValue(node->getPreDelay(), juce::dontSendNotification);
    trimKnob.setValue(node->getOutputTrim(), juce::dontSendNotification);
    loudnessMatchToggle.setToggleState(node->getAutoGain(), juce::dontSendNotification);

    auto current = node->getPreset();
    studioBtn.setToggleState(current == Orion::EnvironmentNode::Preset::Studio, juce::dontSendNotification);
    smallRoomBtn.setToggleState(current == Orion::EnvironmentNode::Preset::SmallRoom, juce::dontSendNotification);
    hallBtn.setToggleState(current == Orion::EnvironmentNode::Preset::Hall, juce::dontSendNotification);
    clubBtn.setToggleState(current == Orion::EnvironmentNode::Preset::Club, juce::dontSendNotification);
    carBtn.setToggleState(current == Orion::EnvironmentNode::Preset::Car, juce::dontSendNotification);
    outdoorBtn.setToggleState(current == Orion::EnvironmentNode::Preset::Outdoor, juce::dontSendNotification);
    phoneBtn.setToggleState(current == Orion::EnvironmentNode::Preset::Phone, juce::dontSendNotification);
}

void AdaptiveSpaceWindow::storeSnapshot(Snapshot& snapshot)
{
    if (node == nullptr) return;

    snapshot.hasData = true;
    snapshot.active = node->isActive();
    snapshot.preset = node->getPreset();
    snapshot.size = node->getSize();
    snapshot.crowd = node->getCrowd();
    snapshot.ambience = node->getAmbience();
    snapshot.intensity = node->getIntensity();
    snapshot.tone = node->getTone();
    snapshot.width = node->getWidth();
    snapshot.grit = node->getGrit();
    snapshot.preDelay = node->getPreDelay();
    snapshot.outputTrim = node->getOutputTrim();
    snapshot.autoGain = node->getAutoGain();

    updateButtonStates();
}

void AdaptiveSpaceWindow::loadSnapshot(const Snapshot& snapshot)
{
    if (node == nullptr || !snapshot.hasData) return;

    node->setActive(snapshot.active);
    node->setPreset(snapshot.preset);
    node->setSize(snapshot.size);
    node->setCrowd(snapshot.crowd);
    node->setAmbience(snapshot.ambience);
    node->setIntensity(snapshot.intensity);
    node->setTone(snapshot.tone);
    node->setWidth(snapshot.width);
    node->setGrit(snapshot.grit);
    node->setPreDelay(snapshot.preDelay);
    node->setOutputTrim(snapshot.outputTrim);
    node->setAutoGain(snapshot.autoGain);
    refreshFromNode();
}

void AdaptiveSpaceWindow::applyQuickMixProfile(const juce::String& profile)
{
    if (node == nullptr) return;

    if (profile == "Vocal") {
        node->setPreset(Orion::EnvironmentNode::Preset::SmallRoom);
        node->setSize(0.34f);
        node->setCrowd(0.52f);
        node->setAmbience(0.23f);
        node->setIntensity(0.56f);
        node->setTone(0.48f);
        node->setWidth(0.58f);
        node->setGrit(0.04f);
        node->setPreDelay(0.26f);
        node->setOutputTrim(0.53f);
    } else if (profile == "Drums") {
        node->setPreset(Orion::EnvironmentNode::Preset::Studio);
        node->setSize(0.26f);
        node->setCrowd(0.68f);
        node->setAmbience(0.19f);
        node->setIntensity(0.69f);
        node->setTone(0.57f);
        node->setWidth(0.70f);
        node->setGrit(0.16f);
        node->setPreDelay(0.14f);
        node->setOutputTrim(0.49f);
    } else if (profile == "Mix Bus") {
        node->setPreset(Orion::EnvironmentNode::Preset::Hall);
        node->setSize(0.47f);
        node->setCrowd(0.45f);
        node->setAmbience(0.16f);
        node->setIntensity(0.52f);
        node->setTone(0.53f);
        node->setWidth(0.62f);
        node->setGrit(0.03f);
        node->setPreDelay(0.24f);
        node->setOutputTrim(0.51f);
    } else if (profile == "FX Send") {
        node->setPreset(Orion::EnvironmentNode::Preset::Hall);
        node->setSize(0.83f);
        node->setCrowd(0.35f);
        node->setAmbience(0.58f);
        node->setIntensity(0.38f);
        node->setTone(0.55f);
        node->setWidth(0.88f);
        node->setGrit(0.02f);
        node->setPreDelay(0.42f);
        node->setOutputTrim(0.42f);
    }

    node->setAutoGain(true);
    node->setActive(true);
    refreshFromNode();
}

void AdaptiveSpaceWindow::updateButtonStates()
{
    auto styleSnapshotBtn = [](juce::TextButton& b, bool hasData, bool isStoreButton) {
        if (isStoreButton) {
            b.setColour(juce::TextButton::buttonColourId, hasData ? juce::Colour(0xFF205C58) : juce::Colour(0xFF2A2A2A));
        } else {
            b.setEnabled(hasData);
            b.setColour(juce::TextButton::buttonColourId, hasData ? juce::Colour(0xFF3A3A3A) : juce::Colour(0xFF202020));
            b.setColour(juce::TextButton::textColourOffId, hasData ? juce::Colours::white : juce::Colour(0xFF666666));
        }
    };

    styleSnapshotBtn(storeABtn, snapshotA.hasData, true);
    styleSnapshotBtn(recallABtn, snapshotA.hasData, false);
    styleSnapshotBtn(storeBBtn, snapshotB.hasData, true);
    styleSnapshotBtn(recallBBtn, snapshotB.hasData, false);
}

void AdaptiveSpaceWindow::paint(juce::Graphics& g)
{

    juce::Colour bgBase = juce::Colour(0xFF10141A);
    juce::ColourGradient grad(bgBase.brighter(0.18f), 0, 0,
                              bgBase.darker(0.32f), 0, (float)getHeight(), false);
    g.setGradientFill(grad);
    g.fillAll();


    g.setColour(juce::Colours::white.withAlpha(0.02f));
    for (int i = 0; i < getWidth(); i += 20) g.drawVerticalLine(i, 0.0f, (float)getHeight());
    for (int i = 0; i < getHeight(); i += 20) g.drawHorizontalLine(i, 0.0f, (float)getWidth());


    auto panel = getLocalBounds().reduced(14).toFloat();
    g.setColour(juce::Colour(0xFF0C1118).withAlpha(0.6f));
    g.fillRoundedRectangle(panel, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(panel, 10.0f, 1.0f);
}

void AdaptiveSpaceWindow::resized()
{
    auto area = getLocalBounds().reduced(20);


    auto header = area.removeFromTop(44);
    auto titleArea = header.removeFromLeft(360);
    titleLabel.setBounds(titleArea.removeFromTop(24));
    subtitleLabel.setBounds(titleArea);
    auto rightArea = header;
    versionLabel.setBounds(rightArea.removeFromTop(18));
    activeToggle.setBounds(rightArea.removeFromBottom(22));

    area.removeFromTop(12);


    presetsLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);

    auto presetArea = area.removeFromTop(80);

    int gap = 10;
    int btnW = (presetArea.getWidth() - (3 * gap)) / 4;
    int btnH = (presetArea.getHeight() - gap) / 2;

    auto row1 = presetArea.removeFromTop(btnH);
    studioBtn.setBounds(row1.removeFromLeft(btnW));
    row1.removeFromLeft(gap);
    smallRoomBtn.setBounds(row1.removeFromLeft(btnW));
    row1.removeFromLeft(gap);
    hallBtn.setBounds(row1.removeFromLeft(btnW));
    row1.removeFromLeft(gap);
    clubBtn.setBounds(row1);

    presetArea.removeFromTop(gap);
    auto row2 = presetArea;
    carBtn.setBounds(row2.removeFromLeft(btnW));
    row2.removeFromLeft(gap);
    outdoorBtn.setBounds(row2.removeFromLeft(btnW));
    row2.removeFromLeft(gap);
    phoneBtn.setBounds(row2.removeFromLeft(btnW));


    area.removeFromTop(10);
    quickMixLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(6);

    auto quickArea = area.removeFromTop(28);
    int quickGap = 8;
    int quickBtnW = (quickArea.getWidth() - (3 * quickGap)) / 4;
    vocalBtn.setBounds(quickArea.removeFromLeft(quickBtnW));
    quickArea.removeFromLeft(quickGap);
    drumsBtn.setBounds(quickArea.removeFromLeft(quickBtnW));
    quickArea.removeFromLeft(quickGap);
    busBtn.setBounds(quickArea.removeFromLeft(quickBtnW));
    quickArea.removeFromLeft(quickGap);
    fxBtn.setBounds(quickArea);

    area.removeFromTop(14);

    int knobH = 92;


    auto knobRow1 = area.removeFromTop(knobH);
    int knobW = knobRow1.getWidth() / 3;

    auto placeKnob = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> r, int index) {
        juce::Rectangle<int> slot(r.getX() + index * knobW, r.getY(), knobW, r.getHeight());
        l.setBounds(slot.removeFromBottom(20));
        s.setBounds(slot.reduced(10));
    };

    placeKnob(sizeKnob, sizeLabel, knobRow1, 0);
    placeKnob(crowdKnob, crowdLabel, knobRow1, 1);
    placeKnob(ambienceKnob, ambienceLabel, knobRow1, 2);

    area.removeFromTop(10);


    auto knobRow2 = area.removeFromTop(knobH);
    placeKnob(intensityKnob, intensityLabel, knobRow2, 0);
    placeKnob(toneKnob, toneLabel, knobRow2, 1);
    placeKnob(widthKnob, widthLabel, knobRow2, 2);

    area.removeFromTop(10);

    auto knobRow3 = area.removeFromTop(knobH);
    placeKnob(gritKnob, gritLabel, knobRow3, 0);
    placeKnob(preDelayKnob, preDelayLabel, knobRow3, 1);
    placeKnob(trimKnob, trimLabel, knobRow3, 2);

    area.removeFromTop(8);
    auto utilArea = area.removeFromTop(28);
    loudnessMatchToggle.setBounds(utilArea.removeFromLeft(160));
    utilArea.removeFromLeft(8);
    resetBtn.setBounds(utilArea.removeFromLeft(84));

    area.removeFromTop(10);
    auto abRow = area.removeFromTop(28);
    int abGap = 8;
    int abW = (abRow.getWidth() - (3 * abGap)) / 4;
    storeABtn.setBounds(abRow.removeFromLeft(abW));
    abRow.removeFromLeft(abGap);
    recallABtn.setBounds(abRow.removeFromLeft(abW));
    abRow.removeFromLeft(abGap);
    storeBBtn.setBounds(abRow.removeFromLeft(abW));
    abRow.removeFromLeft(abGap);
    recallBBtn.setBounds(abRow);
}
