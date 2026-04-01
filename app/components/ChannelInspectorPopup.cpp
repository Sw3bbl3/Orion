#include "ChannelInspectorPopup.h"
#include "../OrionLookAndFeel.h"
#include "orion/PluginManager.h"
#include "PluginWindowManager.h"
#include "orion/SettingsManager.h"

#include "orion/EditorCommands.h"
#include "orion/InstrumentTrack.h"

ChannelInspectorPopup::ChannelInspectorPopup(Orion::AudioEngine& e,
                                           Orion::CommandHistory& history,
                                           std::shared_ptr<Orion::AudioTrack> t)
    : engine(e), commandHistory(history), track(t)
{
    setOpaque(true);

    addAndMakeVisible(titleLabel);
    titleLabel.setText(track->getName() + " INSPECTOR", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions("Segoe UI", 13.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));

    addAndMakeVisible(nameEditor);
    nameEditor.setText(track->getName(), juce::dontSendNotification);
    nameEditor.setJustification(juce::Justification::centred);
    nameEditor.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    nameEditor.onReturnKey = [this] {
        commandHistory.push(std::make_unique<Orion::SetTrackNameCommand>(track, nameEditor.getText().toStdString(), track->getName()));
    };

    auto setupBtn = [this](std::unique_ptr<juce::Button>& btn, const juce::String& text, juce::Colour col, const juce::String& tooltip) {
        btn = std::make_unique<juce::TextButton>(text);
        btn->setColour(juce::TextButton::buttonOnColourId, col);
        btn->setClickingTogglesState(true);
        btn->setTooltip(tooltip);
        addAndMakeVisible(btn.get());
    };

    setupBtn(muteButton, "M", juce::Colour(0xFFE04040), "Mute");
    setupBtn(soloButton, "S", juce::Colour(0xFFE0A040), "Solo");
    setupBtn(armButton, "R", juce::Colour(0xFFE04040), "Record Arm");
    setupBtn(phaseButton, "Ø", juce::Colour(0xFF4080E0), "Invert Phase");
    setupBtn(monoButton, "Mono", juce::Colour(0xFF808080), "Force Mono");

    muteButton->onClick = [this] { commandHistory.push(std::make_unique<Orion::SetTrackMuteCommand>(track, muteButton->getToggleState())); };
    soloButton->onClick = [this] { commandHistory.push(std::make_unique<Orion::SetTrackSoloCommand>(engine, track, soloButton->getToggleState())); };
    armButton->onClick = [this] { commandHistory.push(std::make_unique<Orion::SetTrackArmCommand>(track, armButton->getToggleState())); };
    phaseButton->onClick = [this] { track->setPhaseInverted(phaseButton->getToggleState()); };
    monoButton->onClick = [this] { track->setForceMono(monoButton->getToggleState()); };

    addAndMakeVisible(volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::LinearBar);
    volumeSlider.setRange(0.0, 1.0);
    volumeSlider.setValue(track->getVolume(), juce::dontSendNotification);
    volumeSlider.onValueChange = [this] {
        commandHistory.push(std::make_unique<Orion::SetTrackVolumeCommand>(track, (float)volumeSlider.getValue(), track->getVolume()));
    };

    addAndMakeVisible(panSlider);
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange(-1.0, 1.0);
    panSlider.setValue(track->getPan(), juce::dontSendNotification);
    panSlider.onValueChange = [this] {
        commandHistory.push(std::make_unique<Orion::SetTrackPanCommand>(track, (float)panSlider.getValue(), track->getPan()));
    };

    addAndMakeVisible(volLabel);
    volLabel.setText("VOL", juce::dontSendNotification);
    volLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    volLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    volLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(panLabel);
    panLabel.setText("PAN", juce::dontSendNotification);
    panLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    panLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    panLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(inputSelector);
    updateInputSelector();
    inputSelector.onChange = [this] { track->setInputChannel(inputSelector.getSelectedId() - 2); };

    addAndMakeVisible(outputSelector);
    updateOutputRouting();
    outputSelector.onChange = [this] {
        int id = outputSelector.getSelectedId();
        if (id == 1) engine.routeTrackTo(track, nullptr);
        else {
            auto tracks = engine.getTracks();
            int idx = id - 2;
            if (idx >= 0 && idx < (int)tracks.size()) engine.routeTrackTo(track, tracks[idx]);
        }
    };

    addAndMakeVisible(insertsViewport);
    insertsViewport.setViewedComponent(&insertsContainer, false);
    insertsViewport.setScrollBarsShown(false, true);

    addAndMakeVisible(addInsertButton);
    addInsertButton.setButtonText("+");
    addInsertButton.onClick = [this] { showAddInsertMenu(); };
    addInsertButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    addInsertButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));

    addAndMakeVisible(sendsViewport);
    sendsViewport.setViewedComponent(&sendsContainer, false);
    sendsViewport.setScrollBarsShown(false, true);

    addAndMakeVisible(addSendButton);
    addSendButton.setButtonText("+");
    addSendButton.onClick = [this] {
        juce::PopupMenu m;
        auto tracks = engine.getTracks();
        for (auto& target : tracks) {
            if (target != track && !track->wouldCreateCycle(target)) {
                 m.addItem(target->getName(), [this, target] {
                     track->addSend(target);
                     updateSends();
                     resized();
                 });
            }
        }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(addSendButton));
    };
    addSendButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    addSendButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));

    updateInserts();
    updateSends();

    startTimerHz(30);
}

ChannelInspectorPopup::~ChannelInspectorPopup() {}

void ChannelInspectorPopup::paint(juce::Graphics& g)
{
    auto baseColour = findColour(OrionLookAndFeel::trackHeaderBackgroundColourId);

    // Background with subtle gradient
    juce::ColourGradient bgGrad(baseColour.darker(0.1f), 0.0f, 0.0f, baseColour.darker(0.3f), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    auto bounds = getLocalBounds().toFloat();

    // Header background
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRect(0.0f, 0.0f, (float)getWidth(), 35.0f);

    // Section headers drawing logic
    auto drawSectionHeader = [&](juce::Rectangle<float> r, juce::String title) {
        // Glassy header look
        juce::ColourGradient hGrad(juce::Colours::white.withAlpha(0.05f), 0.0f, r.getY(),
                                   juce::Colours::transparentBlack, 0.0f, r.getBottom(), false);
        g.setGradientFill(hGrad);
        g.fillRect(r);

        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawHorizontalLine((int)r.getBottom(), 0.0f, (float)getWidth());

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions("Segoe UI", 11.0f, juce::Font::bold).withKerningFactor(0.05f));
        g.drawText(title.toUpperCase(), r.reduced(10.0f, 0.0f), juce::Justification::centredLeft, true);
    };

    auto trackHeader = getLocalBounds().toFloat().reduced(10.0f, 0.0f).withY((float)nameEditor.getBottom() + 5.0f).withHeight(22.0f);
    drawSectionHeader(trackHeader, "Track");

    auto insertsHeaderRect = insertsViewport.getBounds().toFloat().withHeight(22.0f).translated(0.0f, -22.0f);
    drawSectionHeader(insertsHeaderRect, "Inserts");

    auto sendsHeader = sendsViewport.getBounds().toFloat().withHeight(22.0f).translated(0.0f, -22.0f);
    drawSectionHeader(sendsHeader, "Sends");

    // Outer border
    g.setColour(findColour(OrionLookAndFeel::accentColourId).withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);
}

void ChannelInspectorPopup::resized()
{
    auto area = getLocalBounds().reduced(2);

    titleLabel.setBounds(area.removeFromTop(35));
    area.reduce(8, 0);

    nameEditor.setBounds(area.removeFromTop(28));
    area.removeFromTop(4);

    // Track Section
    area.removeFromTop(25); // section header space
    auto trackArea = area.removeFromTop(180);

    auto row1 = trackArea.removeFromTop(30);
    int btnW = 32;
    muteButton->setBounds(row1.removeFromLeft(btnW).reduced(2));
    soloButton->setBounds(row1.removeFromLeft(btnW).reduced(2));
    armButton->setBounds(row1.removeFromLeft(btnW).reduced(2));
    phaseButton->setBounds(row1.removeFromLeft(btnW).reduced(2));
    monoButton->setBounds(row1.removeFromLeft(50).reduced(2));

    panLabel.setBounds(row1.removeFromLeft(34).reduced(0, 2));
    panSlider.setBounds(row1.reduced(2));

    trackArea.removeFromTop(8);
    auto volRow = trackArea.removeFromTop(24);
    volLabel.setBounds(volRow.removeFromLeft(40).reduced(0, 2));
    volumeSlider.setBounds(volRow.reduced(2));

    trackArea.removeFromTop(12);
    auto inputRow = trackArea.removeFromTop(24);
    inputSelector.setBounds(inputRow.reduced(2));

    trackArea.removeFromTop(6);
    auto outputRow = trackArea.removeFromTop(24);
    outputSelector.setBounds(outputRow.reduced(2));

    area.removeFromTop(10);

    // Inserts/Sends
    int halfH = area.getHeight() / 2;

    area.removeFromTop(25);
    auto insertsArea = area.removeFromTop(halfH - 25);
    insertsViewport.setBounds(insertsArea);
    addInsertButton.setBounds(insertsArea.withHeight(22).translated(0, -22).removeFromRight(24).reduced(2));

    area.removeFromTop(35);
    auto sendsArea = area;
    sendsViewport.setBounds(sendsArea);
    addSendButton.setBounds(sendsArea.withHeight(22).translated(0, -22).removeFromRight(24).reduced(2));

    // Layout slots
    int slotH = 24;
    int gap = 2;

    int currentY = 0;
    int w = insertsViewport.getWidth();
    for (auto& slot : insertSlots) {
        slot->setBounds(4, currentY, w - 8, slotH);
        currentY += slotH + gap;
    }
    insertsContainer.setSize(w, juce::jmax(insertsViewport.getHeight(), currentY));

    currentY = 0;
    w = sendsViewport.getWidth();
    for (auto& slot : sendSlots) {
        slot->setBounds(4, currentY, w - 8, slotH);
        currentY += slotH + gap;
    }
    sendsContainer.setSize(w, juce::jmax(sendsViewport.getHeight(), currentY));
}

void ChannelInspectorPopup::timerCallback()
{
    if (track) {
        if (track->getEffects()->size() != insertSlots.size()) {
            updateInserts();
            resized();
        }
        if (track->getSends().size() != sendSlots.size()) {
            updateSends();
            resized();
        }

        if (!volumeSlider.isMouseOverOrDragging())
            volumeSlider.setValue(track->getVolume(), juce::dontSendNotification);

        if (!panSlider.isMouseOverOrDragging())
            panSlider.setValue(track->getPan(), juce::dontSendNotification);

        muteButton->setToggleState(track->isMuted(), juce::dontSendNotification);
        soloButton->setToggleState(track->isSolo(), juce::dontSendNotification);
        armButton->setToggleState(track->isArmed(), juce::dontSendNotification);
        phaseButton->setToggleState(track->isPhaseInverted(), juce::dontSendNotification);
        monoButton->setToggleState(track->isForceMono(), juce::dontSendNotification);

        if (nameEditor.getText() != juce::String(track->getName()) && !nameEditor.hasKeyboardFocus(false))
            nameEditor.setText(track->getName(), juce::dontSendNotification);

        if (inputSelector.getNumItems() == 0) updateInputSelector();

        int expectedOutputs = 1 + (int)engine.getTracks().size() - 1;
        if (outputSelector.getNumItems() != expectedOutputs && !outputSelector.isPopupActive())
            updateOutputRouting();
    }
}

void ChannelInspectorPopup::updateInserts()
{
    insertSlots.clear();
    insertsContainer.removeAllChildren();

    auto effects = track->getEffects();
    const float* trackCol = track->getColor();
    auto trackTint = juce::Colour::fromFloatRGBA(trackCol[0], trackCol[1], trackCol[2], 1.0f);

    for (const auto& fx : *effects) {
        auto slot = std::make_unique<InsertSlot>(fx);
        slot->setTrackTint(trackTint);
        slot->onOpenEditor = [fx] { PluginWindowManager::getInstance().showEditor(fx); };
        slot->onRemove = [this, fx] {
            track->removeEffect(fx);
            updateInserts();
            resized();
        };
        insertsContainer.addAndMakeVisible(slot.get());
        insertSlots.push_back(std::move(slot));
    }
}

void ChannelInspectorPopup::updateOutputRouting()
{
    outputSelector.clear();
    outputSelector.addItem("Master", 1);

    auto tracks = engine.getTracks();
    for (int i = 0; i < (int)tracks.size(); ++i) {
        if (tracks[i] != track) {
            outputSelector.addItem(tracks[i]->getName(), i + 2);
        }
    }

    auto target = track->getOutputTarget();
    if (target == nullptr) {
        outputSelector.setSelectedId(1, juce::dontSendNotification);
    } else {
        for(int i=0; i<(int)tracks.size(); ++i) {
            if (tracks[i] == target) {
                outputSelector.setSelectedId(i + 2, juce::dontSendNotification);
                break;
            }
        }
    }
}

void ChannelInspectorPopup::updateInputSelector()
{
    inputSelector.clear();
    bool isInstrument = (dynamic_cast<Orion::InstrumentTrack*>(track.get()) != nullptr);

    if (isInstrument) {
        inputSelector.addItem("MIDI Input", 1);
        inputSelector.setSelectedId(1, juce::dontSendNotification);
        inputSelector.setEnabled(false);
    } else {
        inputSelector.addItem("Stereo In", 1);
        inputSelector.addItem("Input 1", 2);
        inputSelector.addItem("Input 2", 3);
        inputSelector.setSelectedId(track->getInputChannel() + 2, juce::dontSendNotification);
        inputSelector.setEnabled(true);
    }
}

void ChannelInspectorPopup::updateSends()
{
    sendSlots.clear();
    sendsContainer.removeAllChildren();

    auto sends = track->getSends();
    const float* trackCol = track->getColor();
    auto trackTint = juce::Colour::fromFloatRGBA(trackCol[0], trackCol[1], trackCol[2], 1.0f);

    for (size_t i = 0; i < sends.size(); ++i) {
        auto slot = std::make_unique<SendSlot>(sends[i].gainNode, sends[i].target);
        slot->setTrackTint(trackTint);
        slot->onDelete = [this, i] {
            track->removeSend((int)i);
            updateSends();
            resized();
        };
        sendsContainer.addAndMakeVisible(slot.get());
        sendSlots.push_back(std::move(slot));
    }
}

void ChannelInspectorPopup::showAddInsertMenu()
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

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(addInsertButton));
}

bool ChannelInspectorPopup::addInsertPlugin(const Orion::PluginInfo& info)
{
    auto plugin = Orion::PluginManager::loadPlugin(info);
    if (!plugin) return false;
    plugin->setFormat(2, (size_t)engine.getBlockSize());
    plugin->setSampleRate(engine.getSampleRate());
    track->addEffect(plugin);
    updateInserts();
    resized();
    PluginWindowManager::getInstance().showEditor(plugin);
    return true;
}
