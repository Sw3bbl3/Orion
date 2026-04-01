#include "KeyWheelComponent.h"
#include "../OrionLookAndFeel.h"

KeyWheelComponent::KeyWheelComponent(Orion::AudioEngine& e, std::function<void()> callback)
    : engine(e), onKeyChanged(callback)
{
    setupKeys();
    setSize(250, 250);

    juce::String currentKey = engine.getKeySignature();
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i].name == currentKey) {
            selectedIndex = (int)i;
            break;
        }
    }
}

KeyWheelComponent::~KeyWheelComponent() {}

void KeyWheelComponent::setupKeys()
{
    // Circle of Fifths
    juce::String names[] = { "C", "G", "D", "A", "E", "B", "F#", "Db", "Ab", "Eb", "Bb", "F" };
    for (int i = 0; i < 12; ++i) {
        float angle = (float)i * (juce::MathConstants<float>::twoPi / 12.0f) - juce::MathConstants<float>::halfPi;
        keys.push_back({ names[i], angle });
    }
}

void KeyWheelComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto centre = bounds.getCentre();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;
    float innerRadius = radius * 0.4f;

    g.setColour(findColour(OrionLookAndFeel::lcdBackgroundColourId).darker(0.2f));
    g.fillEllipse(bounds.reduced(2));

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawEllipse(bounds.reduced(2), 2.0f);

    float sectionAngle = juce::MathConstants<float>::twoPi / 12.0f;

    for (int i = 0; i < 12; ++i) {
        bool isHovered = (i == hoveredIndex);
        bool isSelected = (i == selectedIndex);

        juce::Path p;
        float startA = keys[i].angle - sectionAngle * 0.5f;
        float endA = keys[i].angle + sectionAngle * 0.5f;

        p.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startA, endA, true);
        p.lineTo(centre.x + std::cos(endA) * innerRadius, centre.y + std::sin(endA) * innerRadius);
        p.addCentredArc(centre.x, centre.y, innerRadius, innerRadius, 0.0f, endA, startA, false);
        p.closeSubPath();

        if (isSelected) {
            g.setColour(findColour(OrionLookAndFeel::accentColourId).withAlpha(0.6f));
            g.fillPath(p);
            g.setColour(findColour(OrionLookAndFeel::accentColourId));
            g.strokePath(p, juce::PathStrokeType(2.0f));
        } else if (isHovered) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillPath(p);
        }

        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.strokePath(p, juce::PathStrokeType(1.0f));

        // Draw text
        float textRadius = (radius + innerRadius) * 0.5f;
        float tx = centre.x + std::cos(keys[i].angle) * textRadius;
        float ty = centre.y + std::sin(keys[i].angle) * textRadius;

        g.setColour(isSelected ? juce::Colours::white : juce::Colours::white.withAlpha(0.7f));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText(keys[i].name, juce::Rectangle<float>(24, 24).withCentre({tx, ty}), juce::Justification::centred, false);
    }

    // Centre label
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText("KEY", bounds.withSizeKeepingCentre(60, 30), juce::Justification::centred, false);
}

void KeyWheelComponent::resized() {}

void KeyWheelComponent::mouseDown(const juce::MouseEvent& e)
{
    int index = getIndexAt(e.getPosition());
    if (index != -1) {
        selectedIndex = index;
        engine.setKeySignature(keys[index].name.toStdString());
        if (onKeyChanged) onKeyChanged();
        repaint();
    }
}

void KeyWheelComponent::mouseMove(const juce::MouseEvent& e)
{
    int index = getIndexAt(e.getPosition());
    if (index != hoveredIndex) {
        hoveredIndex = index;
        repaint();
    }
}

void KeyWheelComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredIndex = -1;
    repaint();
}

int KeyWheelComponent::getIndexAt(juce::Point<int> p)
{
    auto centre = getLocalBounds().getCentre();
    float dx = (float)(p.x - centre.x);
    float dy = (float)(p.y - centre.y);
    float dist = std::sqrt(dx*dx + dy*dy);

    float radius = juce::jmin(getWidth(), getHeight()) * 0.45f;
    float innerRadius = radius * 0.4f;

    if (dist < innerRadius || dist > radius) return -1;

    float angle = std::atan2(dy, dx);
    float sectionAngle = juce::MathConstants<float>::twoPi / 12.0f;

    for (int i = 0; i < 12; ++i) {
        float diff = angle - keys[i].angle;
        while (diff > juce::MathConstants<float>::pi) diff -= juce::MathConstants<float>::twoPi;
        while (diff < -juce::MathConstants<float>::pi) diff += juce::MathConstants<float>::twoPi;

        if (std::abs(diff) < sectionAngle * 0.5f) return i;
    }

    return -1;
}
