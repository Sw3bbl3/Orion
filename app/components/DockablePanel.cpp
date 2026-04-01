#include "DockablePanel.h"
#include "../OrionIcons.h"
#include "../OrionLookAndFeel.h"
#include "../ui/OrionDesignSystem.h"
#include "../ui/OrionUIPrimitives.h"

DockablePanel::DockablePanel(const juce::String& title, juce::Component& c, std::function<void()> closeFn, std::function<void()> detachFn)
    : content(c), onCloseCallback(closeFn), onDetachCallback(detachFn)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText(title, juce::dontSendNotification);

    titleLabel.setFont(Orion::UI::DesignSystem::instance().fonts().sans(13.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, Orion::UI::DesignSystem::instance().currentTheme().colors.textPrimary.withAlpha(0.9f));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("X");
    Orion::UI::Primitives::styleGhostButton(closeButton);

    closeButton.onClick = [this] {
        if (onCloseCallback) onCloseCallback();
    };

    if (onDetachCallback)
    {
        addAndMakeVisible(detachButton);
        juce::Path detachPath = OrionIcons::getDetachIcon();
        detachButton.setShape(detachPath, true, true, false);
        detachButton.setColours(juce::Colours::grey, juce::Colours::white, juce::Colours::white);

        detachButton.onClick = [this] {
            if (onDetachCallback) onDetachCallback();
        };
    }

    addAndMakeVisible(content);
}

DockablePanel::~DockablePanel()
{
}

void DockablePanel::paint(juce::Graphics& g)
{
    const auto& colors = Orion::UI::DesignSystem::instance().currentTheme().colors;
    auto headerArea = getLocalBounds().removeFromTop(24);

    g.setColour(colors.panelBgAlt);
    g.fillRect(headerArea);

    // Subtle separator line
    g.setColour(colors.border);
    g.drawHorizontalLine(24, 0.0f, (float)getWidth());

    // Panel border
    g.setColour(colors.border.withAlpha(0.8f));
    g.drawRect(getLocalBounds(), 1);
}

void DockablePanel::lookAndFeelChanged()
{
    auto col = findColour(juce::Label::textColourId);
    if (detachButton.isVisible()) {
        detachButton.setColours(col.withAlpha(0.5f), col.withAlpha(0.8f), col);
    }
}

void DockablePanel::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(24); // Height of header

    // Layout buttons from right to left
    auto buttonsArea = header.removeFromRight(60);

    closeButton.setBounds(buttonsArea.removeFromRight(24).reduced(4));

    if (detachButton.isVisible())
    {
        detachButton.setBounds(buttonsArea.removeFromRight(24).reduced(6));
    }

    titleLabel.setBounds(header.reduced(8, 0));

    if (content.getParentComponent() == this)
        content.setBounds(area);
}
