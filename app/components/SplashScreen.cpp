#include "SplashScreen.h"
#include "../OrionLookAndFeel.h"
#include "../ui/OrionDesignSystem.h"

OrionSplashScreen::OrionSplashScreen()
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
}

OrionSplashScreen::~OrionSplashScreen()
{
    stopTimer();
}

void OrionSplashScreen::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    const auto& ds = Orion::UI::DesignSystem::instance();
    const auto& colors = ds.currentTheme().colors;

    juce::ColourGradient bgGradient(colors.chromeBg.brighter(0.08f), 0.0f, 0.0f,
                                    colors.appBg.darker(0.05f), 0.0f, area.getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(area, 12.0f);


    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(area.reduced(0.5f), 12.0f, 1.0f);


    if (logo.isValid())
    {

        int logoW = 200;
        int logoH = (int)((float)logo.getHeight() / logo.getWidth() * logoW);

        g.drawImage(logo,
            (int)area.getCentreX() - logoW / 2,
            (int)area.getCentreY() - logoH / 2 - 25,
            logoW, logoH,
            0, 0, logo.getWidth(), logo.getHeight());
    }


    g.setColour(colors.textPrimary.withAlpha(0.95f));
    g.setFont(ds.fonts().sans(22.0f, juce::Font::bold));

    auto textArea = area.withY(area.getCentreY() + 45).withHeight(30).toNearestInt();
    g.drawText("Loading Orion...", textArea, juce::Justification::centred, true);


    g.setColour(colors.textSecondary);
    g.setFont(ds.fonts().sans(14.0f, juce::Font::plain));

    auto statusArea = area.withY(area.getCentreY() + 75).withHeight(20).toNearestInt();
    g.drawText(statusText, statusArea, juce::Justification::centred, true);


    g.setColour(colors.textSecondary.withAlpha(0.8f));
    g.setFont(ds.fonts().sans(12.0f, juce::Font::plain));

    auto versionArea = area.reduced(15).removeFromBottom(20).removeFromRight(100).toNearestInt();
    g.drawText("v" + juce::String(ProjectInfo::versionString), versionArea, juce::Justification::bottomRight, true);



    auto copyrightArea = area.reduced(15).removeFromBottom(20).removeFromLeft(250).toNearestInt();
    g.drawText(juce::String("Copyright ") + juce::String(juce::CharPointer_UTF8("\xc2\xa9")) + " 2026 WayV Software", copyrightArea, juce::Justification::bottomLeft, true);


    if (currentProgress > 0.0f)
    {



        juce::Path clipPath;
        clipPath.addRoundedRectangle(area, 12.0f);

        g.saveState();
        g.reduceClipRegion(clipPath);

        auto progressBarArea = area.removeFromBottom(4.0f);


        g.setColour(juce::Colours::black);
        g.fillRect(progressBarArea);


        auto accent = findColour(OrionLookAndFeel::dashboardAccentColourId);
        if (accent.isTransparent()) accent = juce::Colour(0xFFFFA500);

        g.setColour(accent);
        g.fillRect(progressBarArea.removeFromLeft(progressBarArea.getWidth() * currentProgress));


        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(progressBarArea.removeFromTop(1.0f));

        g.restoreState();
    }
}

void OrionSplashScreen::resized()
{
}

void OrionSplashScreen::timerCallback()
{
    repaint();
}

void OrionSplashScreen::setStatus(const juce::String& status)
{
    statusText = status;
    repaint();
}

void OrionSplashScreen::setProgress(float progress)
{
    currentProgress = progress;
    repaint();
}
