#include "FluxShaperEditor.h"
#include "../OrionLookAndFeel.h"

FluxShaperEditor::FluxShaperEditor(std::shared_ptr<Orion::FluxShaperNode> n)
    : node(n)
{
    points = node->getPoints();

    addAndMakeVisible(mixSlider);
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixSlider.setRange(0.0, 1.0);
    mixSlider.setValue(node->getMix());
    mixSlider.onValueChange = [this] {
        node->setMix((float)mixSlider.getValue());
    };

    addAndMakeVisible(mixLabel);
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setFont(juce::FontOptions(12.0f));

    startTimerHz(30);
    setSize(300, 200);
}

FluxShaperEditor::~FluxShaperEditor() {}

void FluxShaperEditor::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::mixerChassisColourId).darker(0.2f));

    // Grid
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for(int i=1; i<4; ++i) {
        float x = (float)getWidth() * (i/4.0f);
        g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
    }
    g.drawHorizontalLine(getHeight()/2, 0.0f, (float)getWidth());

    // Curve
    g.setColour(findColour(OrionLookAndFeel::accentColourId));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // Fill
    juce::Path fillPath = curvePath;
    fillPath.lineTo((float)getWidth(), (float)getHeight());
    fillPath.lineTo(0.0f, (float)getHeight());
    fillPath.closeSubPath();
    g.setColour(findColour(OrionLookAndFeel::accentColourId).withAlpha(0.2f));
    g.fillPath(fillPath);

    // Points
    for(const auto& p : points) {
        float x = toX(p.x);
        float y = toY(p.y);

        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 4, y - 4, 8, 8);
        g.setColour(juce::Colours::black);
        g.drawEllipse(x - 4, y - 4, 8, 8, 1.0f);
    }
}

void FluxShaperEditor::resized()
{
    mixSlider.setBounds(getWidth() - 50, 10, 40, 40);
    mixLabel.setBounds(getWidth() - 50, 50, 40, 15);
    updatePath();
}

void FluxShaperEditor::updatePath()
{
    curvePath.clear();
    if (points.empty()) return;

    curvePath.startNewSubPath(toX(points[0].x), toY(points[0].y));

    for (size_t i = 0; i < points.size() - 1; ++i) {
        auto& p1 = points[i];
        auto& p2 = points[i+1];

        // Visualize curve with small segments
        int steps = 20;
        for(int s=1; s<=steps; ++s) {
            float t = (float)s / steps;
            float phase = p1.x + t * (p2.x - p1.x);

            // Curve logic duplicated for visualization
            float exponent = 1.0f;
            if (p1.curve > 0.0f) exponent = 1.0f - p1.curve * 0.9f;
            else if (p1.curve < 0.0f) exponent = 1.0f + std::abs(p1.curve) * 4.0f;

            float curvedT = std::pow(t, exponent);
            float y = p1.y + curvedT * (p2.y - p1.y);

            curvePath.lineTo(toX(phase), toY(y));
        }
    }
}

void FluxShaperEditor::timerCallback()
{
    // Check if points changed externally?
}

void FluxShaperEditor::mouseDown(const juce::MouseEvent& e)
{
    draggedPointIndex = -1;
    for(size_t i=0; i<points.size(); ++i) {
        float px = toX(points[i].x);
        float py = toY(points[i].y);
        if (e.position.getDistanceFrom({px, py}) < 8.0f) {
            draggedPointIndex = (int)i;
            return;
        }
    }

    // Add point if clicked on line?
    if (e.mods.isAltDown()) {
        float phase = fromX((float)e.x);
        float gain = fromY((float)e.y);
        points.push_back({phase, gain, 0.0f});
        // sort
        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b){ return a.x < b.x; });
        node->setPoints(points);
        updatePath();
        repaint();
    }
}

void FluxShaperEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedPointIndex != -1) {
        float x = fromX((float)e.x);
        float y = fromY((float)e.y);

        // Constrain X to neighbors
        if (draggedPointIndex > 0) {
            if (x < points[draggedPointIndex-1].x + 0.01f) x = points[draggedPointIndex-1].x + 0.01f;
        } else {
            x = 0.0f; // Start point locked to 0
        }

        if (draggedPointIndex < (int)points.size() - 1) {
            if (x > points[draggedPointIndex+1].x - 0.01f) x = points[draggedPointIndex+1].x - 0.01f;
        } else {
            x = 1.0f; // End point locked to 1
        }

        points[draggedPointIndex].x = x;
        points[draggedPointIndex].y = y;

        node->setPoints(points);
        updatePath();
        repaint();
    }
}

void FluxShaperEditor::mouseUp(const juce::MouseEvent&)
{
    draggedPointIndex = -1;
}
