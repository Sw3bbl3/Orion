#include "ChannelStrip.h"
#include "orion/PluginManager.h"
#include "orion/InstrumentTrack.h"
#include "orion/SamplerNode.h"
#include "orion/GainNode.h"
#include "orion/EQ3Node.h"
#include "orion/DelayNode.h"
#include "orion/ReverbNode.h"
#include "orion/CompressorNode.h"
#include "orion/FluxShaperNode.h"
#include "orion/PrismStackNode.h"
#include "orion/SettingsManager.h"
#include "orion/EnvironmentNode.h"
#include "orion/EditorCommands.h"
#include "PluginWindowManager.h"
#include "../OrionLookAndFeel.h"
#include "AdaptiveSpaceWindow.h"
#include "ChannelInspectorPopup.h"



void MeterComponent::setLevel(float l, float r)
{
    float newMax = std::max(l, r);
    const bool clippedNow = (l > 1.0f || r > 1.0f);
    if (clippedNow) {
        clipHoldCounter = 90;
    } else if (clipHoldCounter > 0) {
        clipHoldCounter--;
    }

    if (newMax > peakHold) {
        peakHold = newMax;
        peakHoldCounter = 60;
    } else if (peakHoldCounter > 0) {
        peakHoldCounter--;
    } else {
        peakHold *= 0.95f;
    }

    if (std::abs(newMax - std::max(levelL, levelR)) > 0.001f || peakHold > 0.001f || clipHoldCounter > 0) {
        levelL = l;
        levelR = r;

        if (newMax > maxPeak) {
            maxPeak = newMax;
        }

        repaint();
    }
}

void MeterComponent::resetPeak() {
    maxPeak = -100.0f;
    clipHoldCounter = 0;
    repaint();
}

void MeterComponent::mouseDown(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    resetPeak();
}

void MeterComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    float textHeight = 14.0f;
    juce::Rectangle<float> textBounds;
    if (showPeakText) {
        textBounds = bounds.removeFromTop(textHeight);
        bounds.removeFromTop(2.0f);
    }

    // Background
    g.setColour(findColour(OrionLookAndFeel::lcdBezelColourId));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    // Dynamic scale width based on component size
    float scaleWidth = (bounds.getWidth() > 30.0f) ? 20.0f : 0.0f;
    auto meterArea = bounds.removeFromRight(bounds.getWidth() - scaleWidth);
    auto scaleArea = bounds;

    // Draw Scale for all meters
    if (scaleWidth > 0.0f) {
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.6f));
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::plain));

        float dbValues[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
        for (float db : dbValues) {
             float norm = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
             norm = juce::jlimit(0.0f, 1.0f, norm);
             float y = meterArea.getBottom() - (norm * (meterArea.getHeight() - 6.0f)) - 3.0f;

             g.drawText(juce::String((int)db), scaleArea.withY(y - 6).withHeight(12), juce::Justification::centredRight, false);

             g.setColour(juce::Colours::white.withAlpha(0.08f));
             g.drawHorizontalLine((int)y, meterArea.getX(), meterArea.getRight());
             g.setColour(findColour(juce::Label::textColourId).withAlpha(0.6f));
        }
    }

    // Meter Background
    g.setColour(findColour(OrionLookAndFeel::meterBackgroundColourId));
    g.fillRoundedRectangle(meterArea.reduced(2.0f), 3.0f);

    // Constrain meter width to look proportionate
    float maxChannelWidth = (meterArea.getWidth() > 30.0f) ? 24.0f : 14.0f;
    float w = (meterArea.getWidth() - 6.0f) / 2.0f;
    if (w > maxChannelWidth) w = maxChannelWidth;
    if (w < 2.0f) w = 2.0f;

    // Center the meters within the available area
    float totalWidth = (w * 2) + 2.0f;
    float startX = meterArea.getX() + (meterArea.getWidth() - totalWidth) * 0.5f;

    auto leftRect = juce::Rectangle<float>(startX, meterArea.getY(), w, meterArea.getHeight());
    auto rightRect = juce::Rectangle<float>(startX + w + 2.0f, meterArea.getY(), w, meterArea.getHeight());

    leftRect = leftRect.reduced(0, 3);
    rightRect = rightRect.reduced(0, 3);

    auto drawChannel = [&](juce::Rectangle<float> area, float level) {
        float db = juce::Decibels::gainToDecibels(level);
        if (db < -60.0f) db = -60.0f;
        float normalizedPos = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
        normalizedPos = juce::jlimit(0.0f, 1.0f, normalizedPos);

        float barHeight = normalizedPos * area.getHeight();
        if (barHeight < 1.0f) barHeight = 0.0f;

        auto filledArea = area.removeFromBottom(barHeight);

        // Gradient Fill
        juce::Colour cStart = findColour(OrionLookAndFeel::meterGradientStartColourId);
        juce::Colour cMid = findColour(OrionLookAndFeel::meterGradientMidColourId);
        juce::Colour cEnd = findColour(OrionLookAndFeel::meterGradientEndColourId);

        juce::ColourGradient grad(cStart, area.getX(), area.getBottom(),
                                  cEnd, area.getX(), area.getY(), false);
        grad.addColour(0.7f, cMid);

        g.setGradientFill(grad);
        g.fillRoundedRectangle(filledArea, 2.0f);

        // Inner Glow / Highlight
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(filledArea.withWidth(filledArea.getWidth() * 0.3f).withX(filledArea.getX() + filledArea.getWidth() * 0.1f));
    };

    if (meterArea.getHeight() > 10.0f) {
        drawChannel(leftRect, levelL);
        drawChannel(rightRect, levelR);

        // Peak Hold Line
        if (peakHold > 0.0001f) {
             float db = juce::Decibels::gainToDecibels(peakHold);
             float norm = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
             norm = juce::jlimit(0.0f, 1.0f, norm);

             float y = meterArea.getBottom() - (norm * (meterArea.getHeight() - 6.0f)) - 3.0f;
             if (y < meterArea.getY() + 3) y = meterArea.getY() + 3;

             g.setColour(findColour(OrionLookAndFeel::meterGradientEndColourId).brighter(0.5f));
             g.fillRect(startX, y, totalWidth, 2.0f);

             // Peak Glow
             juce::Colour peakGlow = findColour(OrionLookAndFeel::meterGradientEndColourId);
             g.setColour(peakGlow.withAlpha(0.3f));
             g.fillRect(startX, y - 3, totalWidth, 6.0f);
        }
    }

    // Glass / Gloss Overlay
    juce::ColourGradient glassGrad(juce::Colours::white.withAlpha(0.15f), meterArea.getX(), meterArea.getY(),
                                   juce::Colours::transparentBlack, meterArea.getX(), meterArea.getCentreY(), false);
    g.setGradientFill(glassGrad);
    g.fillRoundedRectangle(meterArea.reduced(2.0f), 2.0f);

    // Scanlines (Subtle texture)
    g.setColour(juce::Colours::black.withAlpha(0.1f));
    for (int i = 0; i < meterArea.getHeight(); i += 4) {
        g.drawHorizontalLine((int)(meterArea.getY() + i), meterArea.getX(), meterArea.getRight());
    }

    if (showPeakText) {
        float peakDb = juce::Decibels::gainToDecibels(maxPeak);
        if (peakDb < -60.0f) peakDb = -60.0f;

        g.setColour(findColour(juce::Label::textColourId));
        // Smaller, cleaner font for peak readout
        g.setFont(juce::FontOptions("Arial", 10.0f, juce::Font::plain));
        g.drawText(juce::String(peakDb, 1) + " dB", textBounds, juce::Justification::centred, false);
    }

    auto clipRect = getLocalBounds().removeFromTop(12).reduced(2, 0);
    if (showPeakText) clipRect.translate(0, 14);
    const bool clipActive = clipHoldCounter > 0;
    if (clipActive) {
        juce::Colour clipRed = juce::Colour(0xFFFF3B30);
        juce::ColourGradient clipGrad(clipRed.brighter(0.2f), 0.0f, (float)clipRect.getY(),
                                      clipRed.darker(0.2f), 0.0f, (float)clipRect.getBottom(), false);
        g.setGradientFill(clipGrad);
    } else {
        g.setColour(juce::Colours::black.withAlpha(0.4f));
    }
    g.fillRoundedRectangle(clipRect.toFloat(), 2.5f);
    g.setColour(clipActive ? juce::Colours::white : juce::Colours::white.withAlpha(0.45f));
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.drawText("CLIP", clipRect, juce::Justification::centred, false);
}



ChannelStrip::ChannelStrip(Orion::AudioEngine& e, Orion::CommandHistory& history, std::shared_ptr<Orion::AudioTrack> t)
    : engine(e), commandHistory(history), track(t), isMaster(false)
{
    setupControls();
    setName(track->getName());
    if (track) {
        const float* c = track->getColor();
        lastColor[0] = c[0]; lastColor[1] = c[1]; lastColor[2] = c[2];
    }
}

ChannelStrip::ChannelStrip(Orion::AudioEngine& e, Orion::CommandHistory& history, Orion::MasterNode* m)
    : engine(e), commandHistory(history), masterNode(m), isMaster(true)
{
    setupControls();
    setName("Master");
}

ChannelStrip::~ChannelStrip()
{
}

void ChannelStrip::setupControls()
{
    addAndMakeVisible(insertsViewport);
    insertsViewport.setViewedComponent(&insertsContainer, false);
    insertsViewport.setScrollBarsShown(false, true);
    insertsViewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);
    addAndMakeVisible(addInsertButton);
    addAndMakeVisible(meter);
    addInsertButton.setTooltip("Add Insert");
    addInsertButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    addInsertButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white.withAlpha(0.1f));
    addInsertButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
    addInsertButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white.withAlpha(0.9f));
    addInsertButton.onClick = [this] { showAddInsertMenu(); };

    if (isMaster) {
        meter.showPeakText = true;

        if (Orion::SettingsManager::get().presetMonitoringEnabled) {
            setupButton(monitoringButton, "On", juce::Colour(0xFF40E0D0), "Toggle Adaptive Space");
            setupButton(monitoringSettingsButton, "Space", juce::Colour(0xFF888888), "Open Adaptive Space Controls");

            monitoringButton->onClick = [this] {
                if (auto node = engine.getMonitoringNode()) {
                    node->setActive(monitoringButton->getToggleState());
                }
            };

            monitoringSettingsButton->onClick = [this] {
                 showMonitoringSettings();
                 monitoringSettingsButton->setToggleState(false, juce::dontSendNotification);
            };
        }
    }



    bool isInstrument = false;
    if (auto* instTrack = dynamic_cast<Orion::InstrumentTrack*>(track.get())) {
        isInstrument = true;
        auto instrument = instTrack->getInstrument();
        if (auto effect = std::dynamic_pointer_cast<Orion::EffectNode>(instrument)) {
            instrumentSlot = std::make_unique<InsertSlot>(effect);
            addAndMakeVisible(instrumentSlot.get());
            attachContextMenuForwarding(*instrumentSlot);

            instrumentSlot->onOpenEditor = [effect] {
                PluginWindowManager::getInstance().showEditor(effect);
            };

            instrumentSlot->onMenu = [this](juce::PopupMenu& m) {
                if (auto instTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(track)) {
                    m.addSeparator();
                    m.addItem("Switch to Pulsar", [instTrack] {
                        instTrack->setInstrument(std::make_shared<Orion::SynthesizerNode>());
                    });
                    m.addItem("Switch to Comet", [instTrack] {
                        instTrack->setInstrument(std::make_shared<Orion::SamplerNode>());
                    });
                }
            };
        }
    }

    if (!isMaster && !isInstrument) {
        addAndMakeVisible(inputSelector);
        inputSelector.setTooltip("Input Source");
        inputSelector.addItem("Stereo In", 1);
        inputSelector.addItem("Input 1", 2);
        inputSelector.addItem("Input 2", 3);

        inputSelector.onChange = [this] {
            if (track) track->setInputChannel(inputSelector.getSelectedId() - 2);
        };
    }

    if (!isMaster) {
        // Output selector hidden for cleaner UI
    }

    addAndMakeVisible(fader);
    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setRange(-60.0, 6.0, 0.1);
    fader.setSkewFactorFromMidPoint(-12.0);
    fader.setDoubleClickReturnValue(true, 0.0);

    float initGain = isMaster ? masterNode->getVolume() : track->getVolume();
    fader.setValue(juce::Decibels::gainToDecibels(initGain, -60.0f), juce::dontSendNotification);

    fader.onValueChange = [this] {
        float db = (float)fader.getValue();
        float gain = juce::Decibels::decibelsToGain(db);


        if (isMaster) {


            masterNode->setVolume(gain);
        } else {
             commandHistory.push(std::make_unique<Orion::SetTrackVolumeCommand>(
                 track, gain, track->getVolume()
             ));
        }

        juce::String text;
        if (db <= -60.0f) text = "-inf";
        else text = juce::String(db, 1) + " dB";
        valueLabel.setText(text, juce::dontSendNotification);
    };

    if (!isMaster) {
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

        setupButton(muteButton, "M", juce::Colour(0xFFE04040), "Mute");
        setupButton(soloButton, "S", juce::Colour(0xFFE0A040), "Solo");

        // Record button removed from mixer as per request
        // setupButton(recButton, "R", juce::Colour(0xFFE04040), "Record Arm");

        muteButton->onClick = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackMuteCommand>(track, muteButton->getToggleState()));
        };
        soloButton->onClick = [this] {
            commandHistory.push(std::make_unique<Orion::SetTrackSoloCommand>(engine, track, soloButton->getToggleState()));
        };

        // recButton listener removed

        addAndMakeVisible(sendsViewport);
        sendsViewport.setViewedComponent(&sendsContainer, false);
        sendsViewport.setScrollBarsShown(false, true);
        sendsViewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);
        addAndMakeVisible(addSendButton);
        addSendButton.setButtonText("+");


        addSendButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        addSendButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white.withAlpha(0.1f));
        addSendButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
        addSendButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white.withAlpha(0.9f));
        addSendButton.setTooltip("Add Send");

        addSendButton.onClick = [this] {
            juce::PopupMenu m;
            auto tracks = engine.getTracks();
            for (auto& t : tracks) {
                if (t != track && t != track->getOutputTarget()) {
                     m.addItem(t->getName(), [this, t] {
                         track->addSend(t);
                         updateSends();
                         resized();
                     });
                }
            }
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(addSendButton));
        };
        updateSends();
    }

    addAndMakeVisible(nameLabel);
    nameLabel.setText(getName(), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);

    nameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    nameLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);

    nameLabel.setFont(juce::FontOptions("Segoe UI", 11.5f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, findColour(juce::Label::textColourId).withAlpha(0.95f));

    addAndMakeVisible(valueLabel);
    valueLabel.setText("0.0 dB", juce::dontSendNotification);
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setFont(juce::Font(juce::FontOptions("Segoe UI", 10.5f, juce::Font::plain)));
    valueLabel.setColour(juce::Label::textColourId, findColour(juce::Label::textColourId).withAlpha(0.85f));

    attachContextMenuForwarding(insertsViewport);
    attachContextMenuForwarding(addInsertButton);
    attachContextMenuForwarding(meter);
    attachContextMenuForwarding(fader);
    attachContextMenuForwarding(nameLabel);
    attachContextMenuForwarding(valueLabel);

    if (!isMaster) {
        attachContextMenuForwarding(panKnob);
        if (muteButton) attachContextMenuForwarding(*muteButton);
        if (soloButton) attachContextMenuForwarding(*soloButton);
        if (inputSelector.isVisible()) attachContextMenuForwarding(inputSelector);
        attachContextMenuForwarding(sendsViewport);
        attachContextMenuForwarding(addSendButton);
    }

    if (instrumentSlot) {
        attachContextMenuForwarding(*instrumentSlot);
    }

    startTimerHz(60);
}

void ChannelStrip::setupButton(std::unique_ptr<juce::Button>& btn, const juce::String& text, juce::Colour onColour, const juce::String& tooltip)
{
    auto* b = new juce::TextButton(text);
    btn.reset(b);

    b->setColour(juce::TextButton::buttonOnColourId, onColour);

    b->setClickingTogglesState(true);
    b->setTooltip(tooltip);

    addAndMakeVisible(b);
}

void ChannelStrip::attachContextMenuForwarding(juce::Component& component)
{
    component.addMouseListener(this, true);
}

bool ChannelStrip::addInsertPlugin(const Orion::PluginInfo& info)
{
    auto plugin = Orion::PluginManager::loadPlugin(info);
    if (!plugin) return false;

    plugin->setFormat(2, (size_t)engine.getBlockSize());
    plugin->setSampleRate(engine.getSampleRate());

    if (isMaster && masterNode) {
        masterNode->addEffect(plugin);
    } else if (track) {
        track->addEffect(plugin);
    } else {
        return false;
    }

    updateInserts();
    resized();
    PluginWindowManager::getInstance().showEditor(plugin);
    return true;
}

void ChannelStrip::showAddInsertMenu()
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

void ChannelStrip::lookAndFeelChanged()
{

    updateSends();
    updateInserts();
    repaint();
}

void ChannelStrip::paint(juce::Graphics& g)
{
    if (getWidth() <= 0 || getHeight() <= 0) return;

    auto bounds = getLocalBounds().toFloat();
    float cornerSize = 8.0f;

    const bool tintEnabled = Orion::SettingsManager::get().fullTrackColoring && !isMaster && track;
    const juce::Colour trackTint = tintEnabled
        ? juce::Colour::fromFloatRGBA(track->getColor()[0], track->getColor()[1], track->getColor()[2], 1.0f)
        : juce::Colours::transparentBlack;

    // --- Unified Strip Background ---
    juce::Colour baseColour = findColour(OrionLookAndFeel::trackHeaderBackgroundColourId);
    juce::Colour trackCol = juce::Colours::transparentBlack;
    if (!isMaster && track) {
        const float* c = track->getColor();
        trackCol = juce::Colour::fromFloatRGBA(c[0], c[1], c[2], 1.0f);
    }

    // Background with subtle vertical gradient and rounded bottom
    juce::ColourGradient stripGrad(baseColour.brighter(0.02f), 0, 0,
                                   baseColour.darker(0.05f), 0, (float)getHeight(), false);
    g.setGradientFill(stripGrad);

    juce::Path stripPath;
    stripPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize, cornerSize, false, false, true, true);
    g.fillPath(stripPath);

    // Subtle track color tinting if enabled
    if (Orion::SettingsManager::get().fullTrackColoring && !isMaster) {
        g.setColour(trackCol.withAlpha(0.12f));
        g.fillPath(stripPath);
    }

    // Selection Highlight (Elegant)
    if (selected && !isMaster) {
        auto accent = findColour(OrionLookAndFeel::accentColourId);
        g.setColour(accent.withAlpha(0.15f));
        g.fillPath(stripPath);

        // Side highlight glow
        juce::ColourGradient selectionGlow(accent.withAlpha(0.4f), 0, 0,
                                           juce::Colours::transparentBlack, 4, 0, false);
        g.setGradientFill(selectionGlow);
        g.fillRect((float) 0, (float) 0, (float) 4, (float) getHeight() - cornerSize); // Don't glow over the curved bottom edge as much

        juce::ColourGradient selectionGlowR(juce::Colours::transparentBlack, (float)getWidth() - 4, 0,
                                            accent.withAlpha(0.4f), (float)getWidth(), 0, false);
        g.setGradientFill(selectionGlowR);
        g.fillRect((float) getWidth() - 4.0f, (float) 0, (float) 4, (float) getHeight() - cornerSize);
    }

    // Side Borders (Stop before the rounded bottom)
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawVerticalLine(getWidth() - 1, 0.0f, (float)getHeight() - cornerSize);
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawVerticalLine(0, 0.0f, (float)getHeight() - cornerSize);

    // Bottom Border (Rounded)
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.strokePath(stripPath, juce::PathStrokeType(1.0f));

    // --- Section Drawing Helper ---
    auto drawSectionSeparator = [&](float y) {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine((int)y + 1, 0.0f, (float)getWidth());
    };

    auto drawSectionHeader = [&](juce::Rectangle<float> r, juce::String title) {
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRect(r);
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.5f));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold).withKerningFactor(0.05f));
        g.drawText(title.toUpperCase(), r.reduced(6, 0), juce::Justification::centredLeft, true);
    };

    // --- Inserts ---
    if (insertsViewport.isVisible()) {
        auto vBounds = insertsViewport.getBounds().toFloat();
        auto headerRect = vBounds.withHeight(16.0f).translated(0, -16.0f);
        drawSectionHeader(headerRect, "Inserts");

        if (insertSlots.empty() && Orion::SettingsManager::get().hintPolicy.contextualHintsEnabled) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.setFont(juce::FontOptions(10.0f, juce::Font::italic));
            g.drawText("Empty", insertsViewport.getBounds(), juce::Justification::centred, true);
        }
        drawSectionSeparator(vBounds.getBottom() + 2);
    }

    // --- Sends ---
    if (sendsViewport.isVisible()) {
        auto vBounds = sendsViewport.getBounds().toFloat();
        auto headerRect = vBounds.withHeight(16.0f).translated(0, -16.0f);
        drawSectionHeader(headerRect, "Sends");

        if (sendSlots.empty() && !isMaster && Orion::SettingsManager::get().hintPolicy.contextualHintsEnabled) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.setFont(juce::FontOptions(10.0f, juce::Font::italic));
            g.drawText("None", sendsViewport.getBounds(), juce::Justification::centred, true);
        }
        drawSectionSeparator(vBounds.getBottom() + 2);
    }

    // --- Pan/Solo/Mute Block ---
    {
        float blockY = (float)fader.getY() - 105.0f;
        drawSectionSeparator(blockY);
    }

    // --- Fader Area ---
    {
        float faderTopY = (float)fader.getY() - 5.0f;
        drawSectionSeparator(faderTopY);

        // Recessed track look for fader and meter
        auto faderArea = juce::Rectangle<float>(0, faderTopY + 2, (float)getWidth(), (float)getHeight() - (faderTopY + cornerSize + 26));
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.fillRect(faderArea);

        // Fine meter scale lines across the fader area
        g.setColour(juce::Colours::white.withAlpha(0.03f));
        for (int i = 0; i < fader.getHeight(); i += 20)
            g.drawHorizontalLine(fader.getY() + i, 0.0f, (float)getWidth());
    }

    // --- Bottom Name Plate ---
    auto namePlate = juce::Rectangle<float>(0, (float)getHeight() - 24, (float)getWidth(), 24.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));

    juce::Path namePlatePath;
    namePlatePath.addRoundedRectangle(namePlate.getX(), namePlate.getY(), namePlate.getWidth(), namePlate.getHeight(), cornerSize, cornerSize, false, false, true, true);
    g.fillPath(namePlatePath);

    // Top border for name plate
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine((int)namePlate.getY(), 0.0f, (float)getWidth());

    // --- Track Color Strip ---
    auto colorStrip = juce::Rectangle<float>(0, (float)getHeight() - 4, (float)getWidth(), 4.0f);
    if (!isMaster) {
        g.setColour(trackCol);

        juce::Path colorStripPath;
        colorStripPath.addRoundedRectangle(colorStrip.getX(), colorStrip.getY(), colorStrip.getWidth(), colorStrip.getHeight(), cornerSize, cornerSize, false, false, true, true);
        g.fillPath(colorStripPath);

        // Accent glow on color strip if selected
        if (selected) {
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillPath(colorStripPath); // Fill the whole strip for a glow effect
        }
    } else {
        g.setColour(juce::Colours::grey.withAlpha(0.5f));
        juce::Path colorStripPath;
        colorStripPath.addRoundedRectangle(colorStrip.getX(), colorStrip.getY(), colorStrip.getWidth(), colorStrip.getHeight(), cornerSize, cornerSize, false, false, true, true);
        g.fillPath(colorStripPath);
    }
}

void ChannelStrip::mouseDown(const juce::MouseEvent& e) {
    const auto localEvent = e.getEventRelativeTo(this);

    if (localEvent.mods.isPopupMenu() || localEvent.mods.isRightButtonDown()) {
        showStripContextMenu();
        return;
    }

    auto bottomArea = getLocalBounds().withTop(getHeight() - 24);
    if (bottomArea.contains(localEvent.getPosition())) {
        if (!isMaster && track) {
            auto popup = std::make_unique<ChannelInspectorPopup>(engine, commandHistory, track);
            popup->setSize(240, 650);
            juce::CallOutBox::launchAsynchronously(std::move(popup), getScreenBounds().withTop(getScreenY() + getHeight() - 24), nullptr);
            return;
        }

        auto chooser = std::make_unique<juce::ColourSelector>();
        chooser->setSize(300, 400);
        chooser->setName("TrackColorPicker");

        if (track) {
            const float* c = track->getColor();
            chooser->setCurrentColour(juce::Colour::fromFloatRGBA(c[0], c[1], c[2], 1.0f));
        }

        chooser->addChangeListener(this);

        juce::CallOutBox::launchAsynchronously(std::move(chooser), getScreenBounds().withTop(getScreenY() + getHeight() - 24), nullptr);
        return;
    }

    if (!isMaster && !localEvent.mods.isPopupMenu() && !localEvent.mods.isRightButtonDown())
    {
        if (onSelected) onSelected(track);
    }
}

void ChannelStrip::showStripContextMenu()
{
    if (isMaster || !track) return;

    juce::PopupMenu m;
    m.addItem(juce::PopupMenu::Item("Invert Phase")
        .setTicked(track->isPhaseInverted())
        .setAction([this] {
            track->setPhaseInverted(!track->isPhaseInverted());
        }));

    m.addItem(juce::PopupMenu::Item("Force Mono")
        .setTicked(track->isForceMono())
        .setAction([this] {
            track->setForceMono(!track->isForceMono());
        }));

    m.addSeparator();

    juce::PopupMenu inputMenu;
    int currentCh = track->getInputChannel();

    inputMenu.addItem(juce::PopupMenu::Item("Stereo In (1+2)")
        .setTicked(currentCh == -1)
        .setAction([this] { track->setInputChannel(-1); }));

    inputMenu.addItem(juce::PopupMenu::Item("Input 1 (L)")
        .setTicked(currentCh == 0)
        .setAction([this] { track->setInputChannel(0); }));

    inputMenu.addItem(juce::PopupMenu::Item("Input 2 (R)")
        .setTicked(currentCh == 1)
        .setAction([this] { track->setInputChannel(1); }));

    inputMenu.addItem(juce::PopupMenu::Item("Input 3")
        .setTicked(currentCh == 2)
        .setAction([this] { track->setInputChannel(2); }));

    inputMenu.addItem(juce::PopupMenu::Item("Input 4")
        .setTicked(currentCh == 3)
        .setAction([this] { track->setInputChannel(3); }));

    m.addSubMenu("Input Routing", inputMenu);

    juce::PopupMenu outputMenu;
    auto target = track->getOutputTarget();

    outputMenu.addItem(juce::PopupMenu::Item("Master")
        .setTicked(target == nullptr)
        .setAction([this] { engine.routeTrackTo(track, nullptr); }));

    auto tracks = engine.getTracks();
    for (auto& t : tracks) {
        if (t != track) {
            bool isTarget = (target == t);
            outputMenu.addItem(juce::PopupMenu::Item(t->getName())
                .setTicked(isTarget)
                .setAction([this, t] {
                    bool cycle = false;
                    auto checker = t;
                    int depth = 0;
                    while (checker != nullptr && depth < 1000) {
                        if (checker == track) {
                            cycle = true;
                            break;
                        }
                        checker = checker->getOutputTarget();
                        depth++;
                    }

                    if (cycle) {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                            "Routing Error",
                            "Cannot route to this track because it would create a feedback loop.");
                    } else {
                        engine.routeTrackTo(track, t);
                    }
                }));
        }
    }
    m.addSubMenu("Output Routing", outputMenu);

    m.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this));
}

void ChannelStrip::resized()
{


    auto area = getLocalBounds().reduced(4);
    int h = getHeight();



    int faderHeight = 280;
    int bottomMargin = 20;

    if (h < 600) faderHeight = 220;
    if (h < 400) faderHeight = 150;

    int panBlockHeight = 90;
    int labelHeight = 52;
    int topSlotHeight = 28;

    int mandatoryHeight = faderHeight + bottomMargin + panBlockHeight + labelHeight + topSlotHeight;
    int remaining = h - mandatoryHeight;




    nameLabel.setBounds(area.removeFromBottom(24));
    area.removeFromBottom(2);
    valueLabel.setBounds(area.removeFromBottom(16));
    area.removeFromBottom(10);

    auto topSlotArea = area.removeFromTop(24);
    if (instrumentSlot) {
        instrumentSlot->setBounds(topSlotArea);
    } else if (inputSelector.isVisible()) {
        inputSelector.setBounds(topSlotArea);
    } else if (isMaster && monitoringButton) {
        int gap = 4;
        int bW = topSlotArea.getWidth() / 2 - gap/2;
        monitoringButton->setBounds(topSlotArea.getX(), topSlotArea.getY(), bW, topSlotArea.getHeight());
        monitoringSettingsButton->setBounds(topSlotArea.getX() + bW + gap, topSlotArea.getY(), bW, topSlotArea.getHeight());
    }
    area.removeFromTop(4);

    int availableForModules = std::max(0, remaining);


    int headerH = 18;

    int insertH = 0;
    int sendH = 0;

    insertH = availableForModules / 2;
    sendH = availableForModules - insertH - 8;


    if (insertH > 30) {
        auto insertHeaderArea = area.removeFromTop(headerH);
        insertH -= headerH;
        addInsertButton.setVisible(true);
        addInsertButton.setBounds(insertHeaderArea.removeFromRight(20).reduced(2));

        insertsViewport.setVisible(true);
        insertsViewport.setBounds(area.removeFromTop(insertH));
        area.removeFromTop(4);

        int slotH = 22;
        int gap = 2;
        int totalContentHeight = 0;

        for (auto& slot : insertSlots) {
            slot->setVisible(true);
            slot->setBounds(2, totalContentHeight, insertsViewport.getWidth() - 4, slotH);
            totalContentHeight += slotH + gap;
        }
        
        insertsContainer.setSize(insertsViewport.getWidth(), std::max(insertsViewport.getHeight(), totalContentHeight));
    } else {
        insertsViewport.setVisible(false);
        addInsertButton.setVisible(false);
    }


    if (sendH > 30) {
        auto headerArea = area.removeFromTop(headerH);
        sendH -= headerH;

        if (!isMaster) {
            addSendButton.setVisible(true);
            addSendButton.setBounds(headerArea.removeFromRight(20).reduced(2));

            sendsViewport.setVisible(true);
            sendsViewport.setBounds(area.removeFromTop(sendH));
            area.removeFromTop(4);

            int slotH = 22;
            int gap = 2;
            int totalContentHeight = 0;

            for (auto& slot : sendSlots) {
                slot->setVisible(true);
                slot->setBounds(2, totalContentHeight, sendsViewport.getWidth() - 4, slotH);
                totalContentHeight += slotH + gap;
            }
            sendsContainer.setSize(sendsViewport.getWidth(), std::max(sendsViewport.getHeight(), totalContentHeight));
        } else {
            // Master placeholder
            sendsViewport.setVisible(false);
            addSendButton.setVisible(false);
            sendsViewport.setBounds(area.removeFromTop(sendH));
            area.removeFromTop(4);
        }
    } else {
        sendsViewport.setVisible(false);
        addSendButton.setVisible(false);
    }


    if (outputSelector.isVisible()) {
        outputSelector.setBounds(area.removeFromTop(20));
        area.removeFromTop(8);
    } else {
        area.removeFromTop(4);
    }


    int knobSize = 48;
    int btnSize = 24;
    int gap = 4;
    int blockHeight = btnSize + gap + knobSize + gap;
    auto panBlockArea = area.removeFromTop(blockHeight);
    int centerX = panBlockArea.getCentreX();

    if (!isMaster) {
        auto topRow = panBlockArea.removeFromTop(btnSize);
        if (soloButton && muteButton) {
            int bW = 36;
            int startX = centerX - bW - gap/2;
            soloButton->setBounds(startX, topRow.getY(), bW, btnSize);
            muteButton->setBounds(centerX + gap/2, topRow.getY(), bW, btnSize);
        }

        panBlockArea.removeFromTop(gap);

        auto knobRow = panBlockArea.removeFromTop(knobSize);
        panKnob.setBounds(knobRow.withSizeKeepingCentre(knobSize, knobSize));
    } else {
        // Master: Keep panBlockArea empty for alignment
    }

    area.removeFromTop(10);

    // Standardized meter width for all tracks
    int meterWidthInt = 46;

    auto faderArea = area.removeFromLeft(area.getWidth() - (meterWidthInt + 6));
    fader.setBounds(faderArea);

    float meterW = (float)meterWidthInt;
    float meterX = (float)getWidth() - (meterW + 4.0f);

    // Exactly match the fader track height and position
    auto faderBounds = fader.getBounds();
    meter.setBounds((int)meterX, faderBounds.getY(), (int)meterW, faderBounds.getHeight());

    updateInserts();
}

void ChannelStrip::updateInserts()
{
    std::shared_ptr<const std::vector<std::shared_ptr<Orion::EffectNode>>> effects;

    if (isMaster && masterNode) {
        effects = masterNode->getEffects();
    } else if (track) {
        effects = track->getEffects();
    } else {
        return;
    }

    if (effects->size() != insertSlots.size())
    {
        insertSlots.clear();
        insertsContainer.removeAllChildren();

        const bool tintEnabled = Orion::SettingsManager::get().fullTrackColoring && !isMaster && track;
        const juce::Colour trackTint = tintEnabled
            ? juce::Colour::fromFloatRGBA(track->getColor()[0], track->getColor()[1], track->getColor()[2], 1.0f)
            : juce::Colours::transparentBlack;

        for (const auto& fx : *effects)
        {
            auto slot = std::make_unique<InsertSlot>(fx);
            if (tintEnabled) slot->setTrackTint(trackTint);
            attachContextMenuForwarding(*slot);
            slot->onOpenEditor = [fx] {
                PluginWindowManager::getInstance().showEditor(fx);
            };

            slot->onRemove = [this, fx] {
                if (isMaster && masterNode) {
                    masterNode->removeEffect(fx);
                } else if (track) {
                    track->removeEffect(fx);
                }
                updateInserts();
                resized();
                if (auto* parent = getParentComponent()) parent->resized();
                repaint();
            };

            insertsContainer.addAndMakeVisible(slot.get());
            insertSlots.push_back(std::move(slot));
        }
    }
}

void ChannelStrip::timerCallback()
{
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

    float l = 0, r = 0;
    if (isMaster) {
        l = masterNode->getLevelL();
        r = masterNode->getLevelR();

        if (masterNode && masterNode->getEffects()->size() != insertSlots.size()) {
             updateInserts();
        }
    } else {
        l = track->getLevelL();
        r = track->getLevelR();

        int expectedItems = 1;
        auto tracks = engine.getTracks();
        for (auto& t : tracks) if (t != track) expectedItems++;

        if (outputSelector.getNumItems() != expectedItems) {
            updateOutputRouting();
        } else {
            auto currentTarget = track->getOutputTarget();
            int currentId = outputSelector.getSelectedId();
            if (currentTarget == nullptr) {
                if (currentId != 1) outputSelector.setSelectedId(1, juce::dontSendNotification);
            } else {
                int idx = -1;
                for(int i=0; i<(int)tracks.size(); ++i) {
                    if (tracks[i] == currentTarget) { idx = i; break; }
                }
                if (idx != -1) {
                    if (currentId != idx + 2) outputSelector.setSelectedId(idx + 2, juce::dontSendNotification);
                }
            }
        }

        if (muteButton) muteButton->setToggleState(track->isMuted(), juce::dontSendNotification);
        if (soloButton) soloButton->setToggleState(track->isSolo(), juce::dontSendNotification);
        if (recButton) recButton->setToggleState(track->isArmed(), juce::dontSendNotification);
        if (monoButton) monoButton->setToggleState(track->isForceMono(), juce::dontSendNotification);
        if (phaseButton) phaseButton->setToggleState(track->isPhaseInverted(), juce::dontSendNotification);

        if (nameLabel.getText() != juce::String(track->getName()))
            nameLabel.setText(track->getName(), juce::dontSendNotification);

        nameLabel.setColour(juce::Label::textColourId, selected ? findColour(OrionLookAndFeel::accentColourId) : findColour(juce::Label::textColourId));

        if (inputSelector.isVisible() && !inputSelector.isPopupActive()) {
            inputSelector.setSelectedId(track->getInputChannel() + 2, juce::dontSendNotification);
        }

        if (!fader.isMouseOverOrDragging()) {
             float currentGain = track->getVolume();
             float currentDb = juce::Decibels::gainToDecibels(currentGain, -60.0f);
             if (std::abs((float)fader.getValue() - currentDb) > 0.1f) {
                 fader.setValue(currentDb, juce::dontSendNotification);
             }
        }

        if (!panKnob.isMouseOverOrDragging()) {
            if (std::abs((float)panKnob.getValue() - track->getPan()) > 0.01f) {
                panKnob.setValue(track->getPan(), juce::dontSendNotification);
            }
        }

        if (track && track->getEffects()->size() != insertSlots.size()) {
             updateInserts();
             resized();
        }

        if (track && track->getSends().size() != sendSlots.size()) {
             updateSends();
             resized();
        }


        if (auto* instTrack = dynamic_cast<Orion::InstrumentTrack*>(track.get())) {
             auto instrument = instTrack->getInstrument();
             if (instrumentSlot && instrumentSlot->getEffect() != instrument) {
                 if (auto effect = std::dynamic_pointer_cast<Orion::EffectNode>(instrument)) {
                     instrumentSlot = std::make_unique<InsertSlot>(effect);
                     addAndMakeVisible(instrumentSlot.get());
                     attachContextMenuForwarding(*instrumentSlot);
                     instrumentSlot->onOpenEditor = [effect] {
                         PluginWindowManager::getInstance().showEditor(effect);
                     };

                     instrumentSlot->onMenu = [this](juce::PopupMenu& m) {
                         if (auto instTrack = std::dynamic_pointer_cast<Orion::InstrumentTrack>(track)) {
                             m.addSeparator();
                        m.addItem("Switch to Pulsar", [instTrack] {
                            instTrack->setInstrument(std::make_shared<Orion::SynthesizerNode>());
                        });
                        m.addItem("Switch to Comet", [instTrack] {
                            instTrack->setInstrument(std::make_shared<Orion::SamplerNode>());
                        });
                         }
                     };

                     resized();
                 }
             }
        }
    }

    meter.setLevel(l, r);
}

void ChannelStrip::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (auto* selector = dynamic_cast<juce::ColourSelector*>(source))
    {
        if (selector->getName() == "TrackColorPicker" && track)
        {
            auto c = selector->getCurrentColour();
            track->setColor(c.getFloatRed(), c.getFloatGreen(), c.getFloatBlue());
        }
    }
}


bool ChannelStrip::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description.toString().startsWith("Plugin: ");
}

void ChannelStrip::itemDropped(const SourceDetails& dragSourceDetails)
{
    juce::String desc = dragSourceDetails.description.toString();
    juce::String pathAndSubId = desc.substring(8);
    juce::String path = pathAndSubId.upToFirstOccurrenceOf("|", false, false);
    juce::String subId = pathAndSubId.substring(path.length() + 1);

    if (path.startsWith("internal:")) {
        Orion::PluginInfo info;
        info.name = path.fromFirstOccurrenceOf("internal:", false, false).toStdString();
        info.format = Orion::PluginFormat::Internal;
        info.type = Orion::PluginType::Effect;
        if (auto plugin = Orion::PluginManager::loadPlugin(info)) {
            plugin->setFormat(2, (size_t)engine.getBlockSize());
            plugin->setSampleRate(engine.getSampleRate());

            if (isMaster && masterNode) {
                masterNode->addEffect(plugin);
            } else if (track) {
                track->addEffect(plugin);
            }
            updateInserts();
            PluginWindowManager::getInstance().showEditor(plugin);
            resized();
        }
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

    auto plugin = Orion::PluginManager::loadPlugin(info);
    if (plugin)
    {
        plugin->setFormat(2, (size_t)engine.getBlockSize());
        plugin->setSampleRate(engine.getSampleRate());

        if (isMaster && masterNode) {
            masterNode->addEffect(plugin);
            updateInserts();
            PluginWindowManager::getInstance().showEditor(plugin);
            resized();
            return;
        }

        if (!track) return;

        bool droppedOnInstrument = false;
        if (instrumentSlot) {
             auto pt = dragSourceDetails.localPosition;
             if (instrumentSlot->getBounds().contains(pt.toInt())) {
                 droppedOnInstrument = true;
             }
        }

        if (droppedOnInstrument) {
            if (auto* instTrack = dynamic_cast<Orion::InstrumentTrack*>(track.get())) {
                instTrack->setInstrument(plugin);
                PluginWindowManager::getInstance().showEditor(plugin);
                return;
            }
        }

        track->addEffect(plugin);
        updateInserts();
        PluginWindowManager::getInstance().showEditor(plugin);
        resized();
    }
}

void ChannelStrip::updateSends()
{
    if (isMaster) return;

    auto sends = track->getSends();
    if (sends.size() != sendSlots.size()) {
        sendSlots.clear();
        sendsContainer.removeAllChildren();

        const bool tintEnabled = Orion::SettingsManager::get().fullTrackColoring && track;
        const juce::Colour trackTint = tintEnabled
            ? juce::Colour::fromFloatRGBA(track->getColor()[0], track->getColor()[1], track->getColor()[2], 1.0f)
            : juce::Colours::transparentBlack;

        for (size_t i = 0; i < sends.size(); ++i) {
            auto slot = std::make_unique<SendSlot>(sends[i].gainNode, sends[i].target);
            if (tintEnabled) slot->setTrackTint(trackTint);
            attachContextMenuForwarding(*slot);

            slot->onSelect = [this, s=slot.get()] {
                handleSendSelection(s);
            };

            slot->onDelete = [this, s=slot.get()] {
                deleteSend(s);
            };

            sendsContainer.addAndMakeVisible(slot.get());
            sendSlots.push_back(std::move(slot));
        }
    }
}

void ChannelStrip::handleSendSelection(SendSlot* slot)
{
    for (auto& s : sendSlots) {
        s->setSelected(s.get() == slot);
    }
}

void ChannelStrip::deleteSend(SendSlot* slot)
{
    if (!track) return;

    int index = -1;
    for (int i = 0; i < (int)sendSlots.size(); ++i) {
        if (sendSlots[i].get() == slot) {
            index = i;
            break;
        }
    }

    if (index != -1) {
        track->removeSend(index);
        updateSends();
        resized();
        if (auto* p = getParentComponent()) p->resized();
    }
}

void ChannelStrip::updateOutputRouting()
{
    if (isMaster) return;

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

void ChannelStrip::showMonitoringSettings()
{
    if (auto node = engine.getMonitoringNode()) {
        auto panel = std::make_unique<AdaptiveSpaceWindow>(node);


        juce::CallOutBox::launchAsynchronously(
            std::move(panel),
            monitoringSettingsButton->getScreenBounds(),
            nullptr
        );
    }
}
