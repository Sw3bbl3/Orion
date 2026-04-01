#include "OrionUIPrimitives.h"
#include "OrionDesignSystem.h"

namespace Orion::UI::Primitives {

void stylePrimaryButton(juce::TextButton& button)
{
    const auto& ds = DesignSystem::instance();
    const auto& colors = ds.currentTheme().colors;

    button.setColour(juce::TextButton::buttonColourId, colors.accent);
    button.setColour(juce::TextButton::buttonOnColourId, colors.accentStrong);
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    button.setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
}

void styleSecondaryButton(juce::TextButton& button)
{
    const auto& ds = DesignSystem::instance();
    const auto& colors = ds.currentTheme().colors;

    button.setColour(juce::TextButton::buttonColourId, colors.panelBgAlt);
    button.setColour(juce::TextButton::buttonOnColourId, colors.panelBgAlt.brighter(0.2f));
    button.setColour(juce::TextButton::textColourOffId, colors.textPrimary);
    button.setColour(juce::TextButton::textColourOnId, colors.textPrimary);
}

void styleGhostButton(juce::TextButton& button)
{
    const auto& ds = DesignSystem::instance();
    const auto& colors = ds.currentTheme().colors;

    button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    button.setColour(juce::TextButton::buttonOnColourId, colors.panelBgAlt.withAlpha(0.65f));
    button.setColour(juce::TextButton::textColourOffId, colors.textSecondary);
    button.setColour(juce::TextButton::textColourOnId, colors.textPrimary);
}

void styleSectionLabel(juce::Label& label, bool subtle)
{
    const auto& ds = DesignSystem::instance();
    const auto& colors = ds.currentTheme().colors;
    const auto size = subtle ? ds.type.caption : ds.type.label;

    label.setFont(ds.fonts().sans(size, subtle ? juce::Font::plain : juce::Font::bold));
    label.setColour(juce::Label::textColourId, subtle ? colors.textSecondary : colors.textPrimary);
}

void paintPanelSurface(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds,
                       float radius,
                       bool elevated)
{
    const auto& colors = DesignSystem::instance().currentTheme().colors;

    if (elevated)
    {
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 3.0f), radius);
    }

    juce::ColourGradient panelGrad(colors.panelBg.brighter(0.04f), bounds.getTopLeft(),
                                   colors.panelBg.darker(0.05f), bounds.getBottomLeft(), false);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(bounds, radius);
    g.setColour(colors.border.withAlpha(0.8f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
}

void paintWindowBackdrop(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    const auto& colors = DesignSystem::instance().currentTheme().colors;
    juce::ColourGradient bg(colors.appBg.brighter(0.04f), 0.0f, bounds.getY(),
                            colors.appBg.darker(0.05f), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRect(bounds);
}

} // namespace Orion::UI::Primitives

