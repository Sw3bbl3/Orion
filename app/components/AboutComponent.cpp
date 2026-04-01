#include "AboutComponent.h"
#include "../OrionLookAndFeel.h"
#include "orion/ProjectSerializer.h"

AboutComponent::AboutComponent()
{

    juce::File assetsDir("assets");
    auto logoFile = assetsDir.getChildFile("orion").getChildFile("Orion_logo_transpant_bg.png");

    if (!logoFile.existsAsFile())
    {
        auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        logoFile = exeDir.getChildFile("assets/orion/Orion_logo_transpant_bg.png");

        if (!logoFile.existsAsFile())
        {
             logoFile = exeDir.getParentDirectory().getChildFile("assets/orion/Orion_logo_transpant_bg.png");
             if (!logoFile.existsAsFile())
                logoFile = exeDir.getParentDirectory().getParentDirectory().getChildFile("assets/orion/Orion_logo_transpant_bg.png");
        }
    }

    if (logoFile.existsAsFile())
    {
        logo = juce::ImageFileFormat::loadFrom(logoFile);
    }

    startTimerHz(30);
}

AboutComponent::~AboutComponent()
{
    stopTimer();
}

void AboutComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();



    juce::Colour c1 = juce::Colour(0xFF1A1A2E);
    juce::Colour c2 = juce::Colour(0xFF16213E);
    juce::Colour c3 = juce::Colour(0xFF0F3460);


    float phase = gradientOffset;
    float y1 = area.getHeight() * (0.5f + 0.5f * std::sin(phase));
    float y2 = area.getHeight() * (0.5f + 0.5f * std::cos(phase * 0.7f));

    juce::ColourGradient bgGradient(c1, 0.0f, y1, c3, area.getWidth(), y2, false);
    bgGradient.addColour(0.5, c2);

    g.setGradientFill(bgGradient);
    g.fillRect(getLocalBounds());





    if (logo.isValid())
    {
        int logoW = 220;
        int logoH = (int)((float)logo.getHeight() / logo.getWidth() * logoW);

        g.setOpacity(0.9f);
        g.drawImage(logo,
            (int)area.getCentreX() - logoW / 2,
            40,
            logoW, logoH,
            0, 0, logo.getWidth(), logo.getHeight());
        g.setOpacity(1.0f);
    }


    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(32.0f, juce::Font::bold));

    int textY = logo.isValid() ? 160 : 60;

    g.drawText("Orion Beta 1", 0, textY, getWidth(), 40, juce::Justification::centred);


    g.setColour(juce::Colour(0xFFFFA500));
    g.setFont(juce::FontOptions(16.0f, juce::Font::italic));
    g.drawText("Next Gen Audio Production", 0, textY + 35, getWidth(), 20, juce::Justification::centred);


    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("Version 1.0.0 (Build 2026)", 0, textY + 70, getWidth(), 20, juce::Justification::centred);


    g.setColour(juce::Colours::grey);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(juce::String("Copyright ") + juce::String(juce::CharPointer_UTF8("\xc2\xa9")) + " 2026 WayV Software",
               0, getHeight() - 30, getWidth(), 20, juce::Justification::centred);
}

void AboutComponent::resized()
{
}

void AboutComponent::timerCallback()
{
    gradientOffset += 0.02f;
    if (gradientOffset > juce::MathConstants<float>::twoPi) gradientOffset -= juce::MathConstants<float>::twoPi;
    repaint();
}
