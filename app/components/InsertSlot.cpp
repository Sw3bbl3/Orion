#include "InsertSlot.h"
#include "../OrionIcons.h"

class PowerButton : public juce::Button
{
public:
    PowerButton(const juce::Colour& c) : juce::Button("power"), accent(c) { setClickingTogglesState(true); }
    void setAccent(const juce::Colour& c) { accent = c; repaint(); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        juce::ignoreUnused(isButtonDown);
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        bool isActive = getToggleState();

        if (isActive) {
            g.setColour(accent);
            g.fillEllipse(bounds.reduced(1.0f));
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillEllipse(bounds.getX() + bounds.getWidth() * 0.3f, bounds.getY() + bounds.getHeight() * 0.3f, bounds.getWidth() * 0.2f, bounds.getHeight() * 0.2f);
        } else {
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillEllipse(bounds.reduced(1.0f));
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawEllipse(bounds.reduced(1.0f), 1.0f);
        }

        if (isMouseOver) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillEllipse(bounds);
        }
    }
private:
    juce::Colour accent;
};

InsertSlot::InsertSlot(std::shared_ptr<Orion::EffectNode> e) : effect(e)
{
    powerButton = std::make_unique<PowerButton>(juce::Colours::blue);
    powerButton->setToggleState(!effect->isBypassed(), juce::dontSendNotification);
    powerButton->onClick = [this] {
        effect->setBypass(!powerButton->getToggleState());
        repaint();
    };
    addAndMakeVisible(powerButton.get());
    startTimerHz(30);
}

InsertSlot::~InsertSlot()
{
}

void InsertSlot::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    bool isBypassed = effect->isBypassed();
    const juce::String name = effect ? juce::String(effect->getName()) : "Effect";

    auto getAccentForName = [](const juce::String& n) {
        if (n == "EQ3") return juce::Colour(0xFF64D2FF);
        if (n == "Compressor") return juce::Colour(0xFFFF9F0A);
        if (n == "Limiter") return juce::Colour(0xFFFF453A);
        if (n == "Reverb") return juce::Colour(0xFF6E56CF);
        if (n == "Delay") return juce::Colour(0xFF30D158);
        if (n == "Gain") return juce::Colour(0xFF9E9E9E);
        if (n == "Flux Shaper") return juce::Colour(0xFF00C7BE);
        if (n == "Prism Stack") return juce::Colour(0xFFBF5AF2);
        return juce::Colour(0xFF0A84FF);
    };
    const juce::Colour accent = getAccentForName(name);
    if (auto* pb = dynamic_cast<PowerButton*>(powerButton.get()))
        pb->setAccent(accent);

    juce::Colour baseColour = isBypassed ? juce::Colour(0xFF202020) : juce::Colour(0xFF2A2A30);
    juce::ColourGradient bg(baseColour.brighter(0.05f), bounds.getX(), bounds.getY(),
                            baseColour.darker(0.18f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Track tint (full mixer coloring)
    if (! trackTint.isTransparent())
    {
        // Keep tint subtle so plugin type accent + text remains readable
        g.setColour(trackTint.withAlpha(isBypassed ? 0.08f : 0.12f));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Slight "glass" highlight over the tint
        juce::ColourGradient gloss(juce::Colours::white.withAlpha(0.06f), bounds.getX(), bounds.getY(),
                                   juce::Colours::transparentBlack, bounds.getX(), bounds.getCentreY(), false);
        g.setGradientFill(gloss);
        g.fillRoundedRectangle(bounds.reduced(0.5f), 4.0f);
    }


    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);


    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawLine(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY());

    g.setColour(accent.withAlpha(isBypassed ? 0.25f : 0.95f));
    g.fillRoundedRectangle(bounds.getX() + 1.0f, bounds.getY() + 1.0f, 3.0f, bounds.getHeight() - 2.0f, 1.5f);

    g.setColour(isBypassed ? juce::Colours::grey : juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawFittedText(name, bounds.reduced(24, 0).toNearestInt(), juce::Justification::centredLeft, 1);

    const juce::String badge = name.upToFirstOccurrenceOf(" ", false, false).substring(0, 3).toUpperCase();
    auto badgeRect = bounds.toNearestInt().removeFromRight(28).reduced(3, 4);
    g.setColour(accent.withAlpha(isBypassed ? 0.18f : 0.32f));
    g.fillRoundedRectangle(badgeRect.toFloat(), 3.0f);
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText(badge, badgeRect, juce::Justification::centred, false);


    float gripX = bounds.getRight() - 6.0f;
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawVerticalLine((int)gripX, bounds.getY()+4, bounds.getBottom()-4);
    g.drawVerticalLine((int)gripX+2, bounds.getY()+4, bounds.getBottom()-4);
}

void InsertSlot::resized()
{
    powerButton->setBounds(6, (getHeight() - 12) / 2, 12, 12);
}

void InsertSlot::timerCallback()
{
    if (effect) {
        bool active = !effect->isBypassed();
        if (powerButton->getToggleState() != active) {
            powerButton->setToggleState(active, juce::dontSendNotification);
            repaint();
        }
    }
}

void InsertSlot::mouseUp(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        m.addItem("Bypass", true, effect->isBypassed(), [this] {
            effect->setBypass(!effect->isBypassed());
            repaint();
        });

        if (onMenu) onMenu(m);

        m.addSeparator();
        m.addItem("Remove", [this] {
            if (onRemove) onRemove();
        });
        m.showMenuAsync(juce::PopupMenu::Options());
    }
    else
    {
        if (onOpenEditor) onOpenEditor();
    }
}
