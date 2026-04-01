#pragma once

#include <JuceHeader.h>
#include "orion/UiThemeRegistry.h"

#include <map>

namespace Orion::UI {

struct SpacingTokens {
    int xxs = 2;
    int xs = 4;
    int sm = 8;
    int md = 12;
    int lg = 16;
    int xl = 24;
    int xxl = 32;
};

struct RadiusTokens {
    float sm = 4.0f;
    float md = 8.0f;
    float lg = 12.0f;
};

struct MotionTokens {
    int fastMs = 90;
    int normalMs = 150;
    int slowMs = 220;
};

struct TypographyTokens {
    float caption = 11.0f;
    float body = 13.0f;
    float label = 14.0f;
    float title = 18.0f;
    float hero = 24.0f;
};

struct ColorTokens {
    juce::Colour appBg;
    juce::Colour panelBg;
    juce::Colour panelBgAlt;
    juce::Colour chromeBg;
    juce::Colour border;
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    juce::Colour accent;
    juce::Colour accentStrong;
    juce::Colour danger;
    juce::Colour success;
};

struct ThemeTokens {
    std::string id;
    juce::String displayName;
    bool light = false;
    ColorTokens colors;
};

class FontPack {
public:
    FontPack();

    juce::Font sans(float size, int styleFlags = juce::Font::plain) const;
    juce::Font mono(float size, int styleFlags = juce::Font::plain) const;

    bool hasCustomSans() const noexcept { return sansRegular != nullptr; }
    bool hasCustomMono() const noexcept { return monoRegular != nullptr; }

private:
    juce::Typeface::Ptr sansRegular;
    juce::Typeface::Ptr sansBold;
    juce::Typeface::Ptr monoRegular;
};

class DesignSystem {
public:
    static DesignSystem& instance();

    const SpacingTokens spacing;
    const RadiusTokens radius;
    const MotionTokens motion;
    const TypographyTokens type;

    const FontPack& fonts() const noexcept { return fontPack; }
    const ThemeTokens& currentTheme() const;

    std::string resolveThemeId(const juce::String& idOrDisplayName) const;
    void setCurrentTheme(const std::string& themeId);

    juce::StringArray getThemeDisplayNames() const;
    juce::String getDisplayNameForThemeId(const std::string& themeId) const;
    const ThemeTokens& getThemeById(const std::string& themeId) const;

private:
    DesignSystem();

    std::map<std::string, ThemeTokens> themesById;
    std::string currentThemeId = "studio_one_dark";
    FontPack fontPack;
};

} // namespace Orion::UI

