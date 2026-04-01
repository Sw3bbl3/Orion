#include "StatusBarComponent.h"
#include "../OrionIcons.h"
#include "../OrionLookAndFeel.h"
#include "../UiHints.h"
#include "../ui/OrionDesignSystem.h"

StatusBarComponent::StatusButton::StatusButton(const juce::String& name, const juce::Path& p)
    : juce::Button(name), icon(p)
{
    setClickingTogglesState(true);
}

void StatusBarComponent::StatusButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    auto area = getLocalBounds().toFloat().reduced(2.0f);
    bool active = getToggleState();

    if (active)
    {
        g.setColour(findColour(OrionLookAndFeel::accentColourId).withAlpha(0.28f));
        g.fillRoundedRectangle(area, 4.0f);
    }
    else if (isMouseOverButton || isButtonDown)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(area, 4.0f);
    }

    juce::Colour iconColor = active ? juce::Colours::white : juce::Colours::white.withAlpha(0.6f);
    if (isMouseOverButton) iconColor = iconColor.brighter(0.2f);

    g.setColour(iconColor);
    auto iconArea = area.withSizeKeepingCentre(16.0f, 16.0f);
    auto transform = icon.getTransformToScaleToFit(iconArea, true);
    g.fillPath(icon, transform);
}

StatusBarComponent::StatusBarComponent()
{
    inspectorButton = std::make_unique<StatusButton>("Inspector", OrionIcons::getInspectorIcon());
    mixerButton = std::make_unique<StatusButton>("Mixer", OrionIcons::getMixerIcon());
    pianoRollButton = std::make_unique<StatusButton>("Piano Roll", OrionIcons::getPianoRollIcon());
    browserButton = std::make_unique<StatusButton>("Browser", OrionIcons::getBrowserIcon());

    auto setupButton = [this](std::unique_ptr<StatusButton>& b, const juce::String& tooltip, std::function<void()>& callback) {
        b->setTooltip(tooltip);
        b->onClick = [&callback] { if (callback) callback(); };
        addAndMakeVisible(b.get());
    };

    setupButton(inspectorButton, Orion::UiHints::withShortcutText("Show/Hide Inspector", "Alt+1"), onToggleInspector);
    setupButton(mixerButton, Orion::UiHints::withShortcutText("Show/Hide Mixer", "Alt+2"), onToggleMixer);
    setupButton(pianoRollButton, Orion::UiHints::withShortcutText("Show/Hide Piano Roll", "Alt+3"), onTogglePianoRoll);
    setupButton(browserButton, Orion::UiHints::withShortcutText("Show/Hide Browser", "Alt+4"), onToggleBrowser);

    hintLabel.setFont(Orion::UI::DesignSystem::instance().fonts().sans(12.0f));
    hintLabel.setJustificationType(juce::Justification::centredLeft);
    hintLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    addAndMakeVisible(hintLabel);

    startTimer(30);
}

StatusBarComponent::~StatusBarComponent()
{
    stopTimer();
}

void StatusBarComponent::paint(juce::Graphics& g)
{
    const auto& colors = Orion::UI::DesignSystem::instance().currentTheme().colors;
    auto bg = colors.chromeBg.darker(0.05f);
    g.fillAll(bg);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());
}

void StatusBarComponent::resized()
{
    auto area = getLocalBounds().reduced(6, 2);

    const int btnW = 34;
    browserButton->setBounds(area.removeFromRight(btnW));
    area.removeFromRight(6);
    pianoRollButton->setBounds(area.removeFromRight(btnW));
    area.removeFromRight(6);
    mixerButton->setBounds(area.removeFromRight(btnW));
    area.removeFromRight(6);
    inspectorButton->setBounds(area.removeFromRight(btnW));

    area.removeFromRight(12);
    hintLabel.setBounds(area);
}

void StatusBarComponent::timerCallback()
{
    auto* component = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
    juce::String hintText = "";

    if (component)
    {
        if (auto* tc = dynamic_cast<juce::TooltipClient*>(component))
            hintText = tc->getTooltip();

        if (hintText.isEmpty() && component->getParentComponent())
        {
            if (auto* tc = dynamic_cast<juce::TooltipClient*>(component->getParentComponent()))
                hintText = tc->getTooltip();
        }
    }

    if (hintLabel.getText() != hintText)
        hintLabel.setText(hintText, juce::dontSendNotification);
}

void StatusBarComponent::setViewButtonStates(bool showInspector, bool showMixer, bool showPianoRoll, bool showBrowser)
{
    inspectorButton->setToggleState(showInspector, juce::dontSendNotification);
    mixerButton->setToggleState(showMixer, juce::dontSendNotification);
    pianoRollButton->setToggleState(showPianoRoll, juce::dontSendNotification);
    browserButton->setToggleState(showBrowser, juce::dontSendNotification);
}
