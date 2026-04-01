#include "SamplerEditor.h"
#include "../OrionLookAndFeel.h"
#include <algorithm>
#include <cmath>

SamplerEditor::SamplerEditor(Orion::SamplerNode& node)
    : samplerNode(node), waveformPreview(node)
{
    addAndMakeVisible(loadButton);
    loadButton.onClick = [this] {
        auto fileChooser = std::make_shared<juce::FileChooser>("Select Audio File",
            juce::File(),
            "*.wav;*.mp3;*.aiff;*.flac;*.ogg");


        juce::Component::SafePointer<SamplerEditor> safeThis(this);

        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safeThis, fileChooser](const juce::FileChooser& fc) {
                if (safeThis == nullptr) return;

                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    if (safeThis->samplerNode.loadSample(file.getFullPathName().toStdString())) {
                        safeThis->updateSampleName();
                    }
                }
            });
    };

    sampleNameLabel.setText(samplerNode.getSampleName(), juce::dontSendNotification);
    sampleNameLabel.setJustificationType(juce::Justification::centred);
    sampleNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(sampleNameLabel);

    addAndMakeVisible(waveformPreview);

    setupKnob(attackKnob, "Attack");
    setupKnob(decayKnob, "Decay");
    setupKnob(sustainKnob, "Sustain");
    setupKnob(releaseKnob, "Release");
    setupKnob(rootKnob, "Root");
    setupKnob(tuneKnob, "Tune");

    attackKnob.slider.setRange(0.001, 5.0, 0.001);
    decayKnob.slider.setRange(0.001, 5.0, 0.001);
    sustainKnob.slider.setRange(0.0, 1.0, 0.01);
    releaseKnob.slider.setRange(0.001, 5.0, 0.001);
    rootKnob.slider.setRange(0, 127, 1);
    tuneKnob.slider.setRange(-12.0, 12.0, 0.01);

    attackKnob.slider.setValue(samplerNode.getAttack(), juce::dontSendNotification);
    decayKnob.slider.setValue(samplerNode.getDecay(), juce::dontSendNotification);
    sustainKnob.slider.setValue(samplerNode.getSustain(), juce::dontSendNotification);
    releaseKnob.slider.setValue(samplerNode.getRelease(), juce::dontSendNotification);
    rootKnob.slider.setValue(samplerNode.getRootNote(), juce::dontSendNotification);
    tuneKnob.slider.setValue(samplerNode.getTune(), juce::dontSendNotification);

    attackKnob.slider.onValueChange = [this] { samplerNode.setAttack((float)attackKnob.slider.getValue()); };
    decayKnob.slider.onValueChange = [this] { samplerNode.setDecay((float)decayKnob.slider.getValue()); };
    sustainKnob.slider.onValueChange = [this] { samplerNode.setSustain((float)sustainKnob.slider.getValue()); };
    releaseKnob.slider.onValueChange = [this] { samplerNode.setRelease((float)releaseKnob.slider.getValue()); };
    rootKnob.slider.onValueChange = [this] { samplerNode.setRootNote((int)rootKnob.slider.getValue()); };
    tuneKnob.slider.onValueChange = [this] { samplerNode.setTune((float)tuneKnob.slider.getValue()); };

    addAndMakeVisible(attackKnob.label);
    addAndMakeVisible(attackKnob.slider);
    addAndMakeVisible(decayKnob.label);
    addAndMakeVisible(decayKnob.slider);
    addAndMakeVisible(sustainKnob.label);
    addAndMakeVisible(sustainKnob.slider);
    addAndMakeVisible(releaseKnob.label);
    addAndMakeVisible(releaseKnob.slider);
    addAndMakeVisible(rootKnob.label);
    addAndMakeVisible(rootKnob.slider);
    addAndMakeVisible(tuneKnob.label);
    addAndMakeVisible(tuneKnob.slider);

    startLabel.setText("Start", juce::dontSendNotification);
    endLabel.setText("End", juce::dontSendNotification);
    startLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    endLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    startLabel.setJustificationType(juce::Justification::centredLeft);
    endLabel.setJustificationType(juce::Justification::centredLeft);

    startSlider.setRange(0.0, 0.98, 0.001);
    endSlider.setRange(0.02, 1.0, 0.001);
    startSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    endSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    startSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    endSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
    startSlider.setValue(samplerNode.getStart(), juce::dontSendNotification);
    endSlider.setValue(samplerNode.getEnd(), juce::dontSendNotification);

    startSlider.onValueChange = [this] { samplerNode.setStart((float)startSlider.getValue()); };
    endSlider.onValueChange = [this] { samplerNode.setEnd((float)endSlider.getValue()); };

    loopToggle.setToggleState(samplerNode.isLooping(), juce::dontSendNotification);
    loopToggle.onClick = [this] { samplerNode.setLoop(loopToggle.getToggleState()); };

    addAndMakeVisible(startLabel);
    addAndMakeVisible(startSlider);
    addAndMakeVisible(endLabel);
    addAndMakeVisible(endSlider);
    addAndMakeVisible(loopToggle);
}

void SamplerEditor::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xFF0A0F1F), r.getX(), r.getY(),
                            juce::Colour(0xFF1A2038), r.getRight(), r.getBottom(), false);
    g.setGradientFill(bg);
    g.fillAll();

    g.setColour(juce::Colour(0xFF5CC8FF).withAlpha(0.08f));
    g.fillRoundedRectangle(r.reduced(8.0f), 8.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(r.reduced(8.0f), 8.0f, 1.0f);
}

void SamplerEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(28);
    loadButton.setBounds(header.removeFromLeft(110));
    header.removeFromLeft(10);
    sampleNameLabel.setBounds(header);

    area.removeFromTop(6);
    waveformPreview.setBounds(area.removeFromTop(80).reduced(6, 2));

    auto adsrRow = area.removeFromTop(90);
    const int knobW = 70;
    attackKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    attackKnob.slider.setBounds(attackKnob.label.getBounds().withTrimmedTop(18));
    decayKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    decayKnob.slider.setBounds(decayKnob.label.getBounds().withTrimmedTop(18));
    sustainKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    sustainKnob.slider.setBounds(sustainKnob.label.getBounds().withTrimmedTop(18));
    releaseKnob.label.setBounds(adsrRow.removeFromLeft(knobW));
    releaseKnob.slider.setBounds(releaseKnob.label.getBounds().withTrimmedTop(18));

    area.removeFromTop(4);
    auto pitchRow = area.removeFromTop(80);
    rootKnob.label.setBounds(pitchRow.removeFromLeft(knobW));
    rootKnob.slider.setBounds(rootKnob.label.getBounds().withTrimmedTop(18));
    tuneKnob.label.setBounds(pitchRow.removeFromLeft(knobW));
    tuneKnob.slider.setBounds(tuneKnob.label.getBounds().withTrimmedTop(18));
    loopToggle.setBounds(pitchRow.removeFromLeft(90).withTrimmedTop(26));

    area.removeFromTop(4);
    auto startRow = area.removeFromTop(26);
    startLabel.setBounds(startRow.removeFromLeft(50));
    startSlider.setBounds(startRow);
    area.removeFromTop(4);
    auto endRow = area.removeFromTop(26);
    endLabel.setBounds(endRow.removeFromLeft(50));
    endSlider.setBounds(endRow);
}

void SamplerEditor::updateSampleName()
{
    sampleNameLabel.setText(samplerNode.getSampleName(), juce::dontSendNotification);
}

void SamplerEditor::setupKnob(Knob& knob, const juce::String& name)
{
    knob.label.setText(name, juce::dontSendNotification);
    knob.label.setJustificationType(juce::Justification::centred);
    knob.label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));

    knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 54, 18);
    knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF5CC8FF));
    knob.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.15f));
    knob.slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    knob.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    knob.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha(0.35f));
}

SamplerEditor::WaveformPreview::WaveformPreview(Orion::SamplerNode& node)
    : samplerNode(node)
{
    startTimerHz(10);
}

void SamplerEditor::WaveformPreview::timerCallback()
{
    rebuildPath();
    repaint();
}

void SamplerEditor::WaveformPreview::rebuildPath()
{
    waveformPath.clear();
    auto sample = samplerNode.getSample();
    if (!sample || getWidth() <= 0 || getHeight() <= 0) return;

    const uint64_t frames = sample->getLengthFrames();
    if (frames == 0) return;

    const float* data = sample->getChannelData(0);
    if (!data) return;

    auto r = getLocalBounds().toFloat().reduced(6.0f);
    const int w = (int)r.getWidth();
    if (w <= 1) return;
    const double framesPerPixel = (double)frames / (double)w;
    waveformPath.startNewSubPath(r.getX(), r.getCentreY());

    for (int x = 0; x < w; ++x) {
        uint64_t start = (uint64_t)((double)x * framesPerPixel);
        uint64_t end = (uint64_t)((double)(x + 1) * framesPerPixel);
        if (end <= start) end = start + 1;
        if (end > frames) end = frames;

        float peak = 0.0f;
        for (uint64_t i = start; i < end; i += std::max<uint64_t>(1, (end - start) / 16)) {
            peak = std::max(peak, std::abs(data[i]));
        }
        float y = r.getCentreY() - peak * r.getHeight() * 0.45f;
        float y2 = r.getCentreY() + peak * r.getHeight() * 0.45f;
        waveformPath.lineTo(r.getX() + (float)x, y);
        waveformPath.lineTo(r.getX() + (float)x, y2);
    }
}

void SamplerEditor::WaveformPreview::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient bg(juce::Colour(0xFF0E1326), r.getX(), r.getY(),
                            juce::Colour(0xFF181E33), r.getRight(), r.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(r.reduced(4.0f), 6.0f);

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(r.reduced(4.0f), 6.0f, 1.0f);

    if (!waveformPath.isEmpty()) {
        g.setColour(juce::Colour(0xFF7BE7FF).withAlpha(0.9f));
        g.strokePath(waveformPath, juce::PathStrokeType(1.2f));
        g.setColour(juce::Colour(0xFF7BE7FF).withAlpha(0.2f));
        g.strokePath(waveformPath, juce::PathStrokeType(3.0f));
    }
}
