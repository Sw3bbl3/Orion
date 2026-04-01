#include "SynthEditor.h"
#include "../OrionLookAndFeel.h"
#include <algorithm>
#include <cmath>

SynthEditor::SynthEditor(Orion::SynthesizerNode& node)
    : synthNode(node), wavePreview(node)
{
    titleLabel.setText("Pulsar", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    waveformLabel.setText("Wave", juce::dontSendNotification);
    waveformLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(waveformLabel);

    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Triangle", 2);
    waveformBox.addItem("Saw", 3);
    waveformBox.addItem("Square", 4);
    waveformBox.setSelectedId((int)synthNode.getWaveform() + 1);
    waveformBox.onChange = [this] {
        int idx = waveformBox.getSelectedId() - 1;
        synthNode.setWaveform((Orion::Waveform)std::clamp(idx, 0, 3));
    };
    addAndMakeVisible(waveformBox);

    setupKnob(cutoffKnob, "Cutoff");
    setupKnob(resonanceKnob, "Res");
    setupKnob(attackKnob, "Attack");
    setupKnob(decayKnob, "Decay");
    setupKnob(sustainKnob, "Sustain");
    setupKnob(releaseKnob, "Release");
    setupKnob(lfoRateKnob, "LFO Rate");
    setupKnob(lfoDepthKnob, "LFO Depth");

    cutoffKnob.slider.setRange(20.0, 20000.0, 1.0);
    resonanceKnob.slider.setRange(0.0, 1.0, 0.01);
    attackKnob.slider.setRange(0.001, 5.0, 0.001);
    decayKnob.slider.setRange(0.001, 5.0, 0.001);
    sustainKnob.slider.setRange(0.0, 1.0, 0.01);
    releaseKnob.slider.setRange(0.001, 5.0, 0.001);
    lfoRateKnob.slider.setRange(0.1, 20.0, 0.01);
    lfoDepthKnob.slider.setRange(0.0, 1.0, 0.01);

    cutoffKnob.slider.setValue(synthNode.getFilterCutoff(), juce::dontSendNotification);
    resonanceKnob.slider.setValue(synthNode.getFilterResonance(), juce::dontSendNotification);
    attackKnob.slider.setValue(synthNode.getAttack(), juce::dontSendNotification);
    decayKnob.slider.setValue(synthNode.getDecay(), juce::dontSendNotification);
    sustainKnob.slider.setValue(synthNode.getSustain(), juce::dontSendNotification);
    releaseKnob.slider.setValue(synthNode.getRelease(), juce::dontSendNotification);
    lfoRateKnob.slider.setValue(synthNode.getLfoRate(), juce::dontSendNotification);
    lfoDepthKnob.slider.setValue(synthNode.getLfoDepth(), juce::dontSendNotification);

    cutoffKnob.slider.onValueChange = [this] { synthNode.setFilterCutoff((float)cutoffKnob.slider.getValue()); };
    resonanceKnob.slider.onValueChange = [this] { synthNode.setFilterResonance((float)resonanceKnob.slider.getValue()); };
    attackKnob.slider.onValueChange = [this] { synthNode.setAttack((float)attackKnob.slider.getValue()); };
    decayKnob.slider.onValueChange = [this] { synthNode.setDecay((float)decayKnob.slider.getValue()); };
    sustainKnob.slider.onValueChange = [this] { synthNode.setSustain((float)sustainKnob.slider.getValue()); };
    releaseKnob.slider.onValueChange = [this] { synthNode.setRelease((float)releaseKnob.slider.getValue()); };
    lfoRateKnob.slider.onValueChange = [this] { synthNode.setLfoRate((float)lfoRateKnob.slider.getValue()); };
    lfoDepthKnob.slider.onValueChange = [this] { synthNode.setLfoDepth((float)lfoDepthKnob.slider.getValue()); };

    lfoTargetLabel.setText("LFO", juce::dontSendNotification);
    lfoTargetLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(lfoTargetLabel);

    lfoTargetBox.addItem("Off", 1);
    lfoTargetBox.addItem("Pitch", 2);
    lfoTargetBox.addItem("Filter", 3);
    lfoTargetBox.setSelectedId(synthNode.getLfoTarget() + 1);
    lfoTargetBox.onChange = [this] {
        int idx = lfoTargetBox.getSelectedId() - 1;
        synthNode.setLfoTarget(std::clamp(idx, 0, 2));
    };
    addAndMakeVisible(lfoTargetBox);

    auto addKnob = [this](Knob& knob) {
        addAndMakeVisible(knob.label);
        addAndMakeVisible(knob.slider);
    };
    addKnob(cutoffKnob);
    addKnob(resonanceKnob);
    addKnob(attackKnob);
    addKnob(decayKnob);
    addKnob(sustainKnob);
    addKnob(releaseKnob);
    addKnob(lfoRateKnob);
    addKnob(lfoDepthKnob);

    addAndMakeVisible(wavePreview);
}

void SynthEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xFF0B1022), r.getX(), r.getY(),
                            juce::Colour(0xFF1D2340), r.getRight(), r.getBottom(), false);
    g.setGradientFill(bg);
    g.fillAll();

    g.setColour(juce::Colour(0xFF6E56CF).withAlpha(0.08f));
    g.fillRoundedRectangle(r.reduced(8.0f), 8.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(r.reduced(8.0f), 8.0f, 1.0f);
}

void SynthEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto header = area.removeFromTop(26);
    titleLabel.setBounds(header.removeFromLeft(80));
    waveformLabel.setBounds(header.removeFromLeft(46));
    waveformBox.setBounds(header.removeFromLeft(110));

    auto lfoRow = header;
    lfoTargetLabel.setBounds(lfoRow.removeFromLeft(40));
    lfoTargetBox.setBounds(lfoRow.removeFromLeft(90));

    area.removeFromTop(4);
    wavePreview.setBounds(area.removeFromTop(80));

    area.removeFromTop(6);
    auto filterRow = area.removeFromTop(90);
    const int knobW = 70;
    cutoffKnob.label.setBounds(filterRow.removeFromLeft(knobW));
    cutoffKnob.slider.setBounds(cutoffKnob.label.getBounds().withTrimmedTop(18));
    resonanceKnob.label.setBounds(filterRow.removeFromLeft(knobW));
    resonanceKnob.slider.setBounds(resonanceKnob.label.getBounds().withTrimmedTop(18));

    area.removeFromTop(4);
    auto adsrRow = area.removeFromTop(90);
    attackKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    attackKnob.slider.setBounds(attackKnob.label.getBounds().withTrimmedTop(18));
    decayKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    decayKnob.slider.setBounds(decayKnob.label.getBounds().withTrimmedTop(18));
    sustainKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    sustainKnob.slider.setBounds(sustainKnob.label.getBounds().withTrimmedTop(18));
    releaseKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    releaseKnob.slider.setBounds(releaseKnob.label.getBounds().withTrimmedTop(18));

    area.removeFromTop(4);
    auto lfoKnobRow = area.removeFromTop(90);
    lfoRateKnob.label.setBounds(lfoKnobRow.removeFromLeft(knobW));
    lfoRateKnob.slider.setBounds(lfoRateKnob.label.getBounds().withTrimmedTop(18));
    lfoDepthKnob.label.setBounds(lfoKnobRow.removeFromLeft(knobW));
    lfoDepthKnob.slider.setBounds(lfoDepthKnob.label.getBounds().withTrimmedTop(18));
}

void SynthEditor::setupKnob(Knob& knob, const juce::String& name)
{
    knob.label.setText(name, juce::dontSendNotification);
    knob.label.setJustificationType(juce::Justification::centred);
    knob.label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));

    knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
    knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF6E56CF));
    knob.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.15f));
    knob.slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    knob.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    knob.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha(0.35f));
}

SynthEditor::WavePreview::WavePreview(Orion::SynthesizerNode& node)
    : synthNode(node)
{
    startTimerHz(24);
}

void SynthEditor::WavePreview::timerCallback()
{
    phase += 0.06f;
    if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
    repaint();
}

void SynthEditor::WavePreview::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(6.0f);
    juce::ColourGradient bg(juce::Colour(0xFF141A2F), r.getX(), r.getY(),
                            juce::Colour(0xFF1E2440), r.getRight(), r.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(r, 6.0f);

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(r, 6.0f, 1.0f);

    juce::Path wave;
    const int w = (int)r.getWidth();
    if (w <= 1) return;
    wave.startNewSubPath(r.getX(), r.getCentreY());

    for (int x = 0; x < w; ++x) {
        float t = (float)x / (float)w;
        float p = t * juce::MathConstants<float>::twoPi + phase;
        float y = 0.0f;
        switch (synthNode.getWaveform()) {
            case Orion::Waveform::Sine: y = std::sin(p); break;
            case Orion::Waveform::Triangle: y = 2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f))) - 1.0f; break;
            case Orion::Waveform::Square: y = (std::sin(p) >= 0.0f) ? 1.0f : -1.0f; break;
            case Orion::Waveform::Saw: y = 2.0f * (t - std::floor(t + 0.5f)); break;
        }
        float yy = r.getCentreY() + y * r.getHeight() * 0.35f;
        wave.lineTo(r.getX() + (float)x, yy);
    }

    g.setColour(juce::Colour(0xFFD1B3FF).withAlpha(0.9f));
    g.strokePath(wave, juce::PathStrokeType(1.6f));
    g.setColour(juce::Colour(0xFF6E56CF).withAlpha(0.2f));
    g.strokePath(wave, juce::PathStrokeType(4.0f));
}
