#pragma once

#include <JuceHeader.h>
#include <map>

class OrionLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum ColourIds
    {

        mixerChassisColourId = 0x2000000,
        mixerBridgeColourId,
        mixerArmRestColourId,
        mixerArmRestContourColourId,


        timelineBackgroundColourId,
        timelineRulerBackgroundColourId,
        timelineGridColourId,
        timelineRecordingRegionColourId,


        trackHeaderBackgroundColourId,
        trackHeaderPanelColourId,
        trackHeaderSeparatorColourId,


        browserBackgroundColourId,
        browserSidebarBackgroundColourId,
        browserTreeBackgroundColourId,
        browserPreviewBackgroundColourId,


        transportButtonBackgroundColourId,
        transportButtonSymbolColourId,
        lcdBackgroundColourId,
        lcdBezelColourId,
        lcdTextColourId,
        lcdLabelColourId,


        toolButtonBackgroundColourId,
        toolButtonActiveColourId,


        dashboardBackgroundColourId,
        dashboardSidebarBackgroundColourId,
        dashboardCardBackgroundColourId,
        dashboardTextColourId,
        dashboardAccentColourId,

        accentColourId,

        // New IDs for cleaner theming
        knobBaseColourId,
        knobFaceColourId,
        knobOutlineColourId,

        toggleSwitchBackgroundColourId,
        toggleSwitchOutlineColourId,

        meterBackgroundColourId,
        meterGradientStartColourId,
        meterGradientMidColourId,
        meterGradientEndColourId
    };

    struct ThemePalette
    {
        juce::Colour chassis, panel, text, accent, highlight;
        juce::Colour timelineBg, rulerBg, grid, trackHeaderBg, trackHeaderPanel, timelineRecordingRegion, trackHeaderSeparator;
        juce::Colour mixerChassis, mixerBridge, mixerArmRest, mixerArmRestContour;
        juce::Colour browserBg, browserSidebar, browserTree, browserPreview;
        juce::Colour transportBtnBg, transportBtnSym, lcdBg, lcdBezel, lcdText, lcdLabel;
        juce::Colour toolBtnBg, toolBtnActive;
        juce::Colour dashBg, dashSidebar, dashCard, dashText, dashAccent;
        juce::Colour knobBase, knobFace, knobOutline;
        juce::Colour toggleBg, toggleOutline;
        juce::Colour meterBg, meterStart, meterMid, meterEnd;

        // Standard JUCE Colours
        juce::Colour buttonCol, buttonOnCol, textButtonTextOff, textButtonTextOn;
        juce::Colour sliderThumb, sliderTrack, sliderFill, sliderOutline, sliderText;
        juce::Colour scrollThumb, scrollTrack;
        juce::Colour tabBg, tabOutline;
        juce::Colour labelText;
        juce::Colour toggleText, toggleTick;
        juce::Colour groupText, groupOutline;
        juce::Colour popupBg, popupText, popupHighlightBg, popupHighlightText;
        juce::Colour comboBg, comboOutline, comboArrow, comboText;
        juce::Colour listBg, listText;
        juce::Colour treeBg, treeLines, treeText;
        juce::Colour textEditorBg, textEditorText, textEditorHighlight, textEditorOutline, textEditorFocusedOutline;
        juce::Colour resizableWindowBg, documentWindowBg;
    };

    OrionLookAndFeel();

    void setTheme(const juce::String& themeName);
    juce::String getCurrentTheme() const { return currentThemeName; }
    juce::StringArray getAvailableThemes() const;

    void setCustomTheme(const juce::String& name, const ThemePalette& palette);
    ThemePalette getThemePalette(const juce::String& name);
    ThemePalette getCurrentPalette() const;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;


    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;


    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;


    void preparePopupMenuWindow (juce::Component& window) override;
    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;


    void drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;


    void drawScrollbar (juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;


    juce::Font getLabelFont (juce::Label&) override;


    void drawMenuBarBackground (juce::Graphics& g, int width, int height,
                                bool isMouseOverBar,
                                juce::MenuBarComponent& menuBar) override;

    int getMenuBarItemWidth (juce::MenuBarComponent& menuBar,
                             int itemIndex,
                             const juce::String& itemText) override;

    juce::Font getMenuBarFont (juce::MenuBarComponent& menuBar,
                               int itemIndex,
                               const juce::String& itemText) override;

    void drawMenuBarItem (juce::Graphics& g, int width, int height,
                          int itemIndex,
                          const juce::String& itemText,
                          bool isMouseOverItem,
                          bool isMenuOpen,
                          bool isMouseOverBar,
                          juce::MenuBarComponent& menuBar) override;

private:
    juce::String currentThemeName;
    std::map<juce::String, ThemePalette> themes;

    void initThemes();
    void applyTheme(const ThemePalette& palette);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrionLookAndFeel)
};
