#pragma once

#include <JuceHeader.h>

namespace Orion::UI::Primitives {

void stylePrimaryButton(juce::TextButton& button);
void styleSecondaryButton(juce::TextButton& button);
void styleGhostButton(juce::TextButton& button);
void styleSectionLabel(juce::Label& label, bool subtle = false);

void paintPanelSurface(juce::Graphics& g,
                       const juce::Rectangle<float>& bounds,
                       float radius,
                       bool elevated = true);

void paintWindowBackdrop(juce::Graphics& g, const juce::Rectangle<float>& bounds);

} // namespace Orion::UI::Primitives

