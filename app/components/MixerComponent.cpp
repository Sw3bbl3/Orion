#include "MixerComponent.h"
#include "../OrionLookAndFeel.h"

MixerComponent::MixerComponent(Orion::AudioEngine& e, Orion::CommandHistory& history)
    : engine(e), commandHistory(history)
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);


    viewport.setScrollBarsShown(false, true);


    masterStrip = std::make_unique<ChannelStrip>(engine, commandHistory, engine.getMasterNode());
    addAndMakeVisible(masterStrip.get());

    updateTrackList();
    startTimer(500);
}

MixerComponent::~MixerComponent()
{
}

void MixerComponent::updateTrackList()
{
    strips.clear();
    content.removeAllChildren();

    auto tracks = engine.getTracks();
    int x = 0;
    int stripWidth = 112;

    for (auto& track : tracks)
    {
        auto strip = std::make_unique<ChannelStrip>(engine, commandHistory, track);
        strip->setSelected(track == selectedTrack);
        strip->onSelected = [this](std::shared_ptr<Orion::AudioTrack> t) {
            setSelectedTrack(t);
            if (onTrackSelected) onTrackSelected(t);
        };
        content.addAndMakeVisible(strip.get());
        strips.push_back(std::move(strip));
        x += stripWidth;
    }

    resized();
}

void MixerComponent::setSelectedTrack(std::shared_ptr<Orion::AudioTrack> track)
{
    selectedTrack = track;
    for (auto& s : strips)
    {
        if (s) s->setSelected(s->getTrack() == track);
    }
    repaint();
}

void MixerComponent::paint(juce::Graphics& g)
{
    // Professional Sleek Background
    juce::Colour chassisColour = findColour(OrionLookAndFeel::mixerChassisColourId);

    // Clean, modern vertical gradient for the main chassis
    juce::ColourGradient bgGradient(chassisColour.brighter(0.04f), 0, 0,
                                    chassisColour.darker(0.15f), 0, (float)getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Subtle fine-grained texture (simulated)
    g.setColour(juce::Colours::black.withAlpha(0.03f));
    for (int i = 0; i < getWidth(); i += 2)
        g.drawVerticalLine(i, 0.0f, (float)getHeight());

    int bridgeHeight = std::min(280, getHeight() / 2);
    int bottomRailHeight = 40;
    int railY = getHeight() - bottomRailHeight;

    // Modern Header Section (replacing "bridge")
    juce::Colour bridgeBase = findColour(OrionLookAndFeel::mixerBridgeColourId);
    juce::ColourGradient bridgeGrad(bridgeBase.brighter(0.05f), 0, 0,
                                    bridgeBase.darker(0.05f), 0, (float)bridgeHeight, false);
    g.setGradientFill(bridgeGrad);
    g.fillRect(0, 0, getWidth(), bridgeHeight);

    // Sleek separator between header and channel strips
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawHorizontalLine(bridgeHeight, 0.0f, (float)getWidth());
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawHorizontalLine(bridgeHeight + 1, 0.0f, (float)getWidth());

    // Clean Bottom Rail (replacing "arm rest")
    juce::Colour railBase = findColour(OrionLookAndFeel::mixerArmRestColourId);
    juce::ColourGradient railGrad(railBase.brighter(0.03f), 0, (float)railY,
                                  railBase.darker(0.1f), 0, (float)getHeight(), false);
    g.setGradientFill(railGrad);
    g.fillRect(0, railY, getWidth(), bottomRailHeight);

    // Rail Top Edge Definition
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawHorizontalLine(railY, 0.0f, (float)getWidth());
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(railY + 1, 0.0f, (float)getWidth());

    // Master Section Divider (Professional "Gap")
    if (masterStrip) {
        int x = masterStrip->getX();
        int gapWidth = 6;

        // Recessed metal channel look
        juce::Rectangle<float> gap((float)x - (float)gapWidth, 0.0f, (float)gapWidth, (float)getHeight());
        g.setColour(chassisColour.darker(0.6f));
        g.fillRect(gap);

        // Subtle highlights on the edges of the gap
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawVerticalLine(x - gapWidth - 1, 0.0f, (float)getHeight());
        g.drawVerticalLine(x, 0.0f, (float)getHeight());

        // Inner shadow for depth
        juce::ColourGradient gapShadow(juce::Colours::black.withAlpha(0.4f), (float)x - gapWidth, 0,
                                       juce::Colours::transparentBlack, (float)x - gapWidth + 2, 0, false);
        g.setGradientFill(gapShadow);
        g.fillRect(x - gapWidth, 0, 2, getHeight());
    }
}

void MixerComponent::resized()
{
    int stripWidth = 112;
    int separatorWidth = 8;

    auto area = getLocalBounds();

    if (masterStrip) {
        masterStrip->setBounds(area.removeFromRight(stripWidth));
        area.removeFromRight(separatorWidth);
    }

    viewport.setBounds(area);

    int contentHeight = viewport.getHeight();

    int x = 0;
    for (auto& strip : strips)
    {
        strip->setBounds(x, 0, stripWidth, contentHeight);
        x += stripWidth;
    }

    content.setSize(std::max(1, x), contentHeight);

    if (masterStrip) {
        masterStrip->setBounds(masterStrip->getX(), 0, stripWidth, getHeight());
    }
}

void MixerComponent::timerCallback()
{

    if (strips.size() != engine.getTracks().size())
    {
        updateTrackList();
    }
}
