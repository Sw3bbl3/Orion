#include "InternalPluginEditor.h"
#include "orion/SamplerNode.h"
#include "orion/SynthesizerNode.h"
#include "orion/CompressorNode.h"
#include "orion/EQ3Node.h"
#include "orion/AudioTrack.h"
#include <cmath>

namespace {
    juce::Colour getPluginAccent(const std::shared_ptr<Orion::EffectNode>& node)
    {
        if (!node) return juce::Colour(0xFF0A84FF);
        const auto n = juce::String(node->getName());
        if (n == "EQ3") return juce::Colour(0xFF64D2FF);
        if (n == "Compressor") return juce::Colour(0xFFFF9F0A);
        if (n == "Limiter") return juce::Colour(0xFFFF453A);
        if (n == "Reverb") return juce::Colour(0xFF6E56CF);
        if (n == "Delay") return juce::Colour(0xFF30D158);
        if (n == "Gain") return juce::Colour(0xFF9E9E9E);
        if (n == "Flux Shaper") return juce::Colour(0xFF00C7BE);
        if (n == "Prism Stack") return juce::Colour(0xFFBF5AF2);
        if (n == "Pulsar") return juce::Colour(0xFF6E56CF);
        if (n == "Comet") return juce::Colour(0xFF5CC8FF);
        return juce::Colour(0xFF0A84FF);
    }

    class EqVisualizerComponent : public juce::Component, private juce::Timer {
    public:
        explicit EqVisualizerComponent(Orion::EQ3Node& n) : node(n) { startTimerHz(30); }

        void paint(juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced(10.0f);
            g.setColour(juce::Colour(0xFF121418));
            g.fillRoundedRectangle(r, 8.0f);

            const float minDb = -18.0f;
            const float maxDb = 18.0f;
            const float minFreq = 20.0f;
            const float maxFreq = 20000.0f;

            auto freqToX = [&](float f) {
                const float n = (std::log10(juce::jlimit(minFreq, maxFreq, f)) - std::log10(minFreq))
                              / (std::log10(maxFreq) - std::log10(minFreq));
                return r.getX() + n * r.getWidth();
            };
            auto dbToY = [&](float db) {
                return juce::jmap(juce::jlimit(minDb, maxDb, db), maxDb, minDb, r.getY(), r.getBottom());
            };

            g.setColour(juce::Colours::white.withAlpha(0.08f));
            const float dbLines[] = { -18, -12, -6, 0, 6, 12, 18 };
            for (float db : dbLines) {
                const float y = dbToY(db);
                g.drawHorizontalLine((int)y, r.getX(), r.getRight());
                if (db == 0.0f) {
                    g.setColour(juce::Colour(0xFF64D2FF).withAlpha(0.25f));
                    g.drawHorizontalLine((int)y, r.getX(), r.getRight());
                    g.setColour(juce::Colours::white.withAlpha(0.08f));
                }
            }
            const float freqLines[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
            for (float f : freqLines) {
                float x = freqToX(f);
                g.drawVerticalLine((int)x, r.getY(), r.getBottom());
            }

            auto toDb = [](float gain) { return juce::Decibels::gainToDecibels(juce::jmax(0.0001f, gain)); };
            const float lowDb = toDb(node.getParameter(0));
            const float midDb = toDb(node.getParameter(1));
            const float highDb = toDb(node.getParameter(2));

            const auto spectrum = node.getSpectrumData();
            if (!spectrum.empty()) {
                juce::Path analyzer;
                analyzer.startNewSubPath(r.getX(), r.getBottom());
                const int bins = (int)spectrum.size();
                for (int x = 0; x < (int)r.getWidth(); ++x) {
                    const float freq = minFreq * std::pow(maxFreq / minFreq, (float)x / juce::jmax(1.0f, r.getWidth() - 1.0f));
                    const float nyquist = 0.5f * (float)juce::jmax(1.0, node.getSampleRate());
                    const float norm = std::sqrt(juce::jlimit(0.0f, 1.0f, freq / juce::jmax(1.0f, nyquist)));
                    int bin = (int)(norm * (float)(bins - 1));
                    bin = juce::jlimit(0, bins - 1, bin);
                    const float y = juce::jmap(spectrum[(size_t)bin], -96.0f, 12.0f, r.getBottom(), r.getY());
                    analyzer.lineTo(r.getX() + (float)x, y);
                }
                analyzer.lineTo(r.getRight(), r.getBottom());
                analyzer.closeSubPath();

                juce::ColourGradient anGrad(juce::Colour(0xFF64D2FF).withAlpha(0.28f), r.getX(), r.getY(),
                                            juce::Colour(0xFF64D2FF).withAlpha(0.02f), r.getX(), r.getBottom(), false);
                g.setGradientFill(anGrad);
                g.fillPath(analyzer);
            }

            juce::Path curve;
            auto calcLowShelf = [](double fs, double f0, double gain) {
                double A = std::sqrt(gain);
                double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
                double cw = std::cos(w0);
                double sw = std::sin(w0);
                double alpha = sw / 2.0 * std::sqrt(2.0);
                return std::array<double, 6> {
                    A * ((A + 1.0) - (A - 1.0) * cw + 2.0 * std::sqrt(A) * alpha),
                    2.0 * A * ((A - 1.0) - (A + 1.0) * cw),
                    A * ((A + 1.0) - (A - 1.0) * cw - 2.0 * std::sqrt(A) * alpha),
                    (A + 1.0) + (A - 1.0) * cw + 2.0 * std::sqrt(A) * alpha,
                    -2.0 * ((A - 1.0) + (A + 1.0) * cw),
                    (A + 1.0) + (A - 1.0) * cw - 2.0 * std::sqrt(A) * alpha
                };
            };
            auto calcPeak = [](double fs, double f0, double gain, double q) {
                double A = std::sqrt(gain);
                double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
                double cw = std::cos(w0);
                double sw = std::sin(w0);
                double alpha = sw / (2.0 * q);
                return std::array<double, 6> {
                    1.0 + alpha * A, -2.0 * cw, 1.0 - alpha * A,
                    1.0 + alpha / A, -2.0 * cw, 1.0 - alpha / A
                };
            };
            auto calcHighShelf = [](double fs, double f0, double gain) {
                double A = std::sqrt(gain);
                double w0 = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
                double cw = std::cos(w0);
                double sw = std::sin(w0);
                double alpha = sw / 2.0 * std::sqrt(2.0);
                return std::array<double, 6> {
                    A * ((A + 1.0) + (A - 1.0) * cw + 2.0 * std::sqrt(A) * alpha),
                    -2.0 * A * ((A - 1.0) + (A + 1.0) * cw),
                    A * ((A + 1.0) + (A - 1.0) * cw - 2.0 * std::sqrt(A) * alpha),
                    (A + 1.0) - (A - 1.0) * cw + 2.0 * std::sqrt(A) * alpha,
                    2.0 * ((A - 1.0) - (A + 1.0) * cw),
                    (A + 1.0) - (A - 1.0) * cw - 2.0 * std::sqrt(A) * alpha
                };
            };
            auto magnitudeAt = [](const std::array<double, 6>& c, double omega) {
                std::complex<double> z1 = std::exp(std::complex<double>(0.0, -omega));
                std::complex<double> z2 = std::exp(std::complex<double>(0.0, -2.0 * omega));
                std::complex<double> num = c[0] + c[1] * z1 + c[2] * z2;
                std::complex<double> den = c[3] + c[4] * z1 + c[5] * z2;
                return (float)std::abs(num / den);
            };

            const double fs = juce::jmax(22050.0, node.getSampleRate());
            auto lowC = calcLowShelf(fs, 250.0, juce::jmax(0.0001f, node.getParameter(0)));
            auto midC = calcPeak(fs, 1000.0, juce::jmax(0.0001f, node.getParameter(1)), 1.0);
            auto highC = calcHighShelf(fs, 4000.0, juce::jmax(0.0001f, node.getParameter(2)));

            for (int x = 0; x < (int)r.getWidth(); ++x) {
                const float freq = minFreq * std::pow(maxFreq / minFreq, (float)x / juce::jmax(1.0f, r.getWidth() - 1.0f));
                const double omega = 2.0 * juce::MathConstants<double>::pi * (double)freq / fs;
                const float mag = magnitudeAt(lowC, omega) * magnitudeAt(midC, omega) * magnitudeAt(highC, omega);
                const float db = juce::Decibels::gainToDecibels(juce::jmax(1.0e-6f, mag));
                const float y = dbToY(db);
                if (x == 0) curve.startNewSubPath(r.getX(), y);
                else curve.lineTo(r.getX() + (float)x, y);
            }

            g.setColour(juce::Colour(0xFF64D2FF).withAlpha(0.95f));
            g.strokePath(curve, juce::PathStrokeType(2.2f));
            g.setColour(juce::Colour(0xFF64D2FF).withAlpha(0.18f));
            g.strokePath(curve, juce::PathStrokeType(6.0f));

            auto drawHandle = [&g](float x, float y, juce::Colour c, bool selected) {
                g.setColour(c.withAlpha(0.90f));
                g.fillEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f);
                g.setColour(selected ? juce::Colours::white : juce::Colours::black.withAlpha(0.8f));
                g.drawEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f, 1.3f);
            };

            const float xLow = freqToX(250.0f);
            const float xMid = freqToX(1000.0f);
            const float xHigh = freqToX(4000.0f);
            drawHandle(xLow, dbToY(lowDb), juce::Colour(0xFF30D158), activeBand == 0);
            drawHandle(xMid, dbToY(midDb), juce::Colour(0xFF64D2FF), activeBand == 1);
            drawHandle(xHigh, dbToY(highDb), juce::Colour(0xFFFF9F0A), activeBand == 2);

            g.setColour(juce::Colours::white.withAlpha(0.66f));
            g.setFont(juce::FontOptions(10.0f));
            g.drawText("20", (int)freqToX(20) - 8, (int)r.getBottom() - 14, 20, 12, juce::Justification::centred);
            g.drawText("100", (int)freqToX(100) - 12, (int)r.getBottom() - 14, 24, 12, juce::Justification::centred);
            g.drawText("1k", (int)freqToX(1000) - 10, (int)r.getBottom() - 14, 20, 12, juce::Justification::centred);
            g.drawText("10k", (int)freqToX(10000) - 12, (int)r.getBottom() - 14, 24, 12, juce::Justification::centred);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            const auto r = getLocalBounds().toFloat().reduced(10.0f);
            auto freqToX = [&r](float f) {
                constexpr float minFreq = 20.0f;
                constexpr float maxFreq = 20000.0f;
                const float n = (std::log10(juce::jlimit(minFreq, maxFreq, f)) - std::log10(minFreq))
                              / (std::log10(maxFreq) - std::log10(minFreq));
                return r.getX() + n * r.getWidth();
            };
            const float xLow = freqToX(250.0f);
            const float xMid = freqToX(1000.0f);
            const float xHigh = freqToX(4000.0f);
            const float distances[3] = { std::abs(e.position.x - xLow), std::abs(e.position.x - xMid), std::abs(e.position.x - xHigh) };
            activeBand = 0;
            if (distances[1] < distances[0]) activeBand = 1;
            if (distances[2] < distances[activeBand]) activeBand = 2;
            applyYToBand(e.position.y);
        }

        void mouseDrag(const juce::MouseEvent& e) override { applyYToBand(e.position.y); }
        void mouseUp(const juce::MouseEvent&) override { activeBand = -1; repaint(); }

    private:
        Orion::EQ3Node& node;
        int activeBand = -1;

        void applyYToBand(float y)
        {
            if (activeBand < 0) return;
            const auto r = getLocalBounds().toFloat().reduced(10.0f);
            const float clampedY = juce::jlimit(r.getY(), r.getBottom(), y);
            const float db = juce::jmap(clampedY, r.getBottom(), r.getY(), -18.0f, 18.0f);
            const float gain = juce::Decibels::decibelsToGain(db);
            node.setParameter(activeBand, juce::jlimit(0.0f, 4.0f, gain));
            repaint();
        }

        void timerCallback() override { repaint(); }
    };

    class GenericPluginVisualizer : public juce::Component, private juce::Timer {
    public:
        explicit GenericPluginVisualizer(std::shared_ptr<Orion::EffectNode> n) : node(std::move(n)) { startTimerHz(30); }

        void paint(juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced(10.0f);
            const auto accent = getPluginAccent(node);

            juce::ColourGradient bg(juce::Colour(0xFF121318), r.getX(), r.getY(), juce::Colour(0xFF1A1D24), r.getRight(), r.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(r, 8.0f);

            juce::Path wave;
            wave.startNewSubPath(r.getX(), r.getCentreY());
            const int count = juce::jmax(1, node ? node->getParameterCount() : 1);
            float amount = 0.0f;
            for (int i = 0; i < count; ++i) amount += node->getParameter(i);
            amount = amount / (float)count;
            const float amp = juce::jlimit(6.0f, r.getHeight() * 0.45f, amount * 20.0f + 8.0f);

            for (int x = 0; x < (int)r.getWidth(); ++x) {
                const float phase = (float)x * 0.05f + animPhase;
                const float y = r.getCentreY() + std::sin(phase) * amp * 0.22f + std::sin(phase * 0.33f) * amp * 0.15f;
                wave.lineTo(r.getX() + (float)x, y);
            }

            g.setColour(accent.withAlpha(0.92f));
            g.strokePath(wave, juce::PathStrokeType(2.0f));
            g.setColour(accent.withAlpha(0.15f));
            g.strokePath(wave, juce::PathStrokeType(6.0f));
        }

    private:
        std::shared_ptr<Orion::EffectNode> node;
        float animPhase = 0.0f;
        void timerCallback() override { animPhase += 0.08f; repaint(); }
    };
}

InternalPluginEditor::InternalPluginEditor(std::shared_ptr<Orion::EffectNode> node,
                                           Orion::AudioEngine* e,
                                           Orion::AudioTrack* owner)
    : effectNode(node), engine(e), ownerTrack(owner)
{
    createControls();


    if (auto* comp = dynamic_cast<Orion::CompressorNode*>(effectNode.get()))
    {
        sidechainLabel = std::make_unique<juce::Label>("scLabel", "Sidechain:");
        sidechainLabel->setJustificationType(juce::Justification::centredRight);
        sidechainLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(sidechainLabel.get());

        sidechainSelector = std::make_unique<juce::ComboBox>();
        addAndMakeVisible(sidechainSelector.get());

        rebuildSidechainSelector();

        sidechainSelector->onChange = [this, comp] {
            const int id = sidechainSelector->getSelectedId();
            if (id <= 1) {
                comp->setSidechainSource(nullptr);
                return;
            }

            const int optionIndex = id - 2;
            if (optionIndex >= 0 && optionIndex < (int)sidechainTrackOptions.size()) {
                auto* selectedTrack = sidechainTrackOptions[(size_t)optionIndex];
                bool trackStillExists = false;
                if (engine && selectedTrack) {
                    auto tracks = engine->getTracks();
                    for (const auto& t : tracks) {
                        if (t.get() == selectedTrack) {
                            trackStillExists = true;
                            break;
                        }
                    }
                }

                if (trackStillExists && selectedTrack != ownerTrack) {
                    comp->setSidechainSource(selectedTrack);
                    return;
                }
            }

            comp->setSidechainSource(nullptr);
            rebuildSidechainSelector();
            sidechainSelector->setSelectedId(1, juce::dontSendNotification);
        };
    }


    if (auto* sampler = dynamic_cast<Orion::SamplerNode*>(effectNode.get())) {
        samplerEditor = std::make_unique<SamplerEditor>(*sampler);
        addAndMakeVisible(samplerEditor.get());
    }

    if (auto* synth = dynamic_cast<Orion::SynthesizerNode*>(effectNode.get())) {
        synthEditor = std::make_unique<SynthEditor>(*synth);
        addAndMakeVisible(synthEditor.get());
    }

    if (auto* eq = dynamic_cast<Orion::EQ3Node*>(effectNode.get())) {
        pluginVisualizer = std::make_unique<EqVisualizerComponent>(*eq);
        addAndMakeVisible(pluginVisualizer.get());
    } else if (effectNode) {
        pluginVisualizer = std::make_unique<GenericPluginVisualizer>(effectNode);
        addAndMakeVisible(pluginVisualizer.get());
    }

    setSize(560, 420);
}

InternalPluginEditor::~InternalPluginEditor()
{
}

void InternalPluginEditor::rebuildSidechainSelector()
{
    if (!sidechainSelector) return;

    sidechainSelector->clear();
    sidechainTrackOptions.clear();
    sidechainSelector->addItem("None", 1);

    auto* comp = dynamic_cast<Orion::CompressorNode*>(effectNode.get());
    if (!engine || !comp) {
        sidechainSelector->setSelectedId(1, juce::dontSendNotification);
        return;
    }

    Orion::AudioNode* currentSource = comp->getSidechainSource();
    int selectedId = 1;
    int id = 2;

    auto tracks = engine->getTracks();
    for (auto& t : tracks)
    {
        if (!t || t.get() == ownerTrack) continue;
        sidechainSelector->addItem(t->getName(), id);
        sidechainTrackOptions.push_back(t.get());
        if (t.get() == currentSource) selectedId = id;
        ++id;
    }

    if (currentSource == ownerTrack) {
        comp->setSidechainSource(nullptr);
        selectedId = 1;
    }

    sidechainSelector->setSelectedId(selectedId, juce::dontSendNotification);
}

void InternalPluginEditor::createControls()
{
    if (!effectNode) return;

    int count = effectNode->getParameterCount();
    for (int i = 0; i < count; ++i)
    {
        auto control = std::make_unique<ParameterControl>(effectNode->getParameterName(i));


        auto range = effectNode->getParameterRange(i);
        control->slider.setRange(range.minVal, range.maxVal, range.step);
        control->slider.setValue(effectNode->getParameter(i), juce::dontSendNotification);


        control->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        control->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        control->slider.setNumDecimalPlacesToDisplay(2);
        const auto accent = getPluginAccent(effectNode);
        control->slider.setColour(juce::Slider::rotarySliderFillColourId, accent.withAlpha(0.9f));
        control->slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.14f));
        control->slider.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.95f));
        control->slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha(0.32f));
        control->slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.92f));



        control->slider.onValueChange = [this, i] {
             if (effectNode && i < (int)controls.size()) {
                 effectNode->setParameter(i, (float)controls[i]->slider.getValue());
             }
        };

        addAndMakeVisible(control.get());
        controls.push_back(std::move(control));
    }
}

void InternalPluginEditor::paint(juce::Graphics& g)
{
    const auto accent = getPluginAccent(effectNode);
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient grad(juce::Colour(0xFF23242A), r.getCentreX(), 0.0f, juce::Colour(0xFF17181D), r.getCentreX(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillAll();

    g.setColour(accent.withAlpha(0.08f));
    g.fillRoundedRectangle(r.reduced(10.0f), 8.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(r.reduced(10.0f), 8.0f, 1.0f);

    g.setColour(accent.withAlpha(0.95f));
    g.fillRect(12.0f, 12.0f, 4.0f, 16.0f);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText(effectNode ? juce::String(effectNode->getName()) : juce::String("Plugin"), 22, 8, getWidth() - 30, 22, juce::Justification::centredLeft, false);
}

void InternalPluginEditor::resized()
{
    auto area = getLocalBounds().reduced(20);


    if (sidechainSelector) {
        auto row = area.removeFromTop(24);
        sidechainLabel->setBounds(row.removeFromLeft(80));
        row.removeFromLeft(10);
        sidechainSelector->setBounds(row.removeFromLeft(200));
        area.removeFromTop(10);
    }


    if (samplerEditor) {
        samplerEditor->setBounds(area.removeFromTop(230));
        area.removeFromTop(10);
    }

    if (synthEditor) {
        synthEditor->setBounds(area.removeFromTop(250));
        area.removeFromTop(10);
    }

    if (pluginVisualizer) {
        pluginVisualizer->setBounds(area.removeFromTop(130));
        area.removeFromTop(12);
    }


    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    for (auto& c : controls)
    {
        fb.items.add(juce::FlexItem(*c).withWidth(100).withHeight(120).withMargin(10));
    }

    fb.performLayout(area);
}


InternalPluginEditor::ParameterControl::ParameterControl(const juce::String& name)
{
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(label);
    addAndMakeVisible(slider);
}

void InternalPluginEditor::ParameterControl::resized()
{
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(20));
    slider.setBounds(area.reduced(5));
}
