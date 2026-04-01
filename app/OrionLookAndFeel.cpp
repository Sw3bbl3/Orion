#include "OrionLookAndFeel.h"
#include "ui/OrionDesignSystem.h"
#include "orion/UiThemeRegistry.h"

OrionLookAndFeel::OrionLookAndFeel()
{
    initThemes();
    setTheme("Deep Space");
}

juce::StringArray OrionLookAndFeel::getAvailableThemes() const
{
    juce::StringArray names;
    for (const auto& pair : themes)
        names.add(pair.first);
    names.sort(true);
    return names;
}

void OrionLookAndFeel::setTheme(const juce::String& themeName)
{
    auto selectedName = themeName;
    if (themes.find(selectedName) == themes.end())
    {
        const auto resolvedId = Orion::UiThemeRegistry::resolveThemeId(themeName.toStdString());
        selectedName = Orion::UiThemeRegistry::getDisplayName(resolvedId);
    }

    if (themes.find(selectedName) != themes.end())
    {
        currentThemeName = selectedName;
        applyTheme(themes[selectedName]);
        Orion::UI::DesignSystem::instance().setCurrentTheme(Orion::UiThemeRegistry::resolveThemeId(selectedName.toStdString()));
    }
}

void OrionLookAndFeel::setCustomTheme(const juce::String& name, const ThemePalette& palette)
{
    themes[name] = palette;
}

OrionLookAndFeel::ThemePalette OrionLookAndFeel::getThemePalette(const juce::String& name)
{
    if (themes.count(name)) return themes[name];
    return themes["Deep Space"];
}

OrionLookAndFeel::ThemePalette OrionLookAndFeel::getCurrentPalette() const
{
    if (themes.count(currentThemeName)) return themes.at(currentThemeName);
    return themes.at("Deep Space");
}

void OrionLookAndFeel::applyTheme(const ThemePalette& p)
{
    // Custom Colours
    setColour(mixerChassisColourId, p.mixerChassis);
    setColour(mixerBridgeColourId, p.mixerBridge);
    setColour(mixerArmRestColourId, p.mixerArmRest);
    setColour(mixerArmRestContourColourId, p.mixerArmRestContour);

    setColour(timelineBackgroundColourId, p.timelineBg);
    setColour(timelineRulerBackgroundColourId, p.rulerBg);
    setColour(timelineGridColourId, p.grid);
    setColour(timelineRecordingRegionColourId, p.timelineRecordingRegion);

    setColour(trackHeaderBackgroundColourId, p.trackHeaderBg);
    setColour(trackHeaderPanelColourId, p.trackHeaderPanel);
    setColour(trackHeaderSeparatorColourId, p.trackHeaderSeparator);

    setColour(browserBackgroundColourId, p.browserBg);
    setColour(browserSidebarBackgroundColourId, p.browserSidebar);
    setColour(browserTreeBackgroundColourId, p.browserTree);
    setColour(browserPreviewBackgroundColourId, p.browserPreview);

    setColour(transportButtonBackgroundColourId, p.transportBtnBg);
    setColour(transportButtonSymbolColourId, p.transportBtnSym);
    setColour(lcdBackgroundColourId, p.lcdBg);
    setColour(lcdBezelColourId, p.lcdBezel);
    setColour(lcdTextColourId, p.lcdText);
    setColour(lcdLabelColourId, p.lcdLabel);

    setColour(toolButtonBackgroundColourId, p.toolBtnBg);
    setColour(toolButtonActiveColourId, p.toolBtnActive);

    setColour(dashboardBackgroundColourId, p.dashBg);
    setColour(dashboardSidebarBackgroundColourId, p.dashSidebar);
    setColour(dashboardCardBackgroundColourId, p.dashCard);
    setColour(dashboardTextColourId, p.dashText);
    setColour(dashboardAccentColourId, p.dashAccent);

    setColour(accentColourId, p.accent);

    setColour(knobBaseColourId, p.knobBase);
    setColour(knobFaceColourId, p.knobFace);
    setColour(knobOutlineColourId, p.knobOutline);

    setColour(toggleSwitchBackgroundColourId, p.toggleBg);
    setColour(toggleSwitchOutlineColourId, p.toggleOutline);

    // Standard JUCE Colours
    setColour(juce::ResizableWindow::backgroundColourId, p.resizableWindowBg);
    setColour(juce::DocumentWindow::backgroundColourId, p.documentWindowBg);

    setColour(juce::TextEditor::backgroundColourId, p.textEditorBg);
    setColour(juce::TextEditor::textColourId, p.textEditorText);
    setColour(juce::TextEditor::highlightColourId, p.textEditorHighlight);
    setColour(juce::TextEditor::outlineColourId, p.textEditorOutline);
    setColour(juce::TextEditor::focusedOutlineColourId, p.textEditorFocusedOutline);

    setColour(juce::TextButton::buttonColourId, p.buttonCol);
    setColour(juce::TextButton::buttonOnColourId, p.buttonOnCol);
    setColour(juce::TextButton::textColourOffId, p.textButtonTextOff);
    setColour(juce::TextButton::textColourOnId, p.textButtonTextOn);

    setColour(juce::Slider::thumbColourId, p.sliderThumb);
    setColour(juce::Slider::trackColourId, p.sliderTrack);
    setColour(juce::Slider::rotarySliderFillColourId, p.sliderFill);
    setColour(juce::Slider::rotarySliderOutlineColourId, p.sliderOutline);
    setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxTextColourId, p.sliderText);

    setColour(juce::ScrollBar::thumbColourId, p.scrollThumb);
    setColour(juce::ScrollBar::trackColourId, p.scrollTrack);

    setColour(juce::TabbedComponent::backgroundColourId, p.tabBg);
    setColour(juce::TabbedComponent::outlineColourId, p.tabOutline);

    setColour(juce::Label::textColourId, p.labelText);

    setColour(juce::ToggleButton::textColourId, p.toggleText);
    setColour(juce::ToggleButton::tickColourId, p.toggleTick);

    setColour(juce::GroupComponent::textColourId, p.groupText);
    setColour(juce::GroupComponent::outlineColourId, p.groupOutline);

    setColour(juce::PopupMenu::backgroundColourId, p.popupBg);
    setColour(juce::PopupMenu::textColourId, p.popupText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, p.popupHighlightBg);
    setColour(juce::PopupMenu::highlightedTextColourId, p.popupHighlightText);

    setColour(juce::ComboBox::backgroundColourId, p.comboBg);
    setColour(juce::ComboBox::outlineColourId, p.comboOutline);
    setColour(juce::ComboBox::arrowColourId, p.comboArrow);
    setColour(juce::ComboBox::textColourId, p.comboText);

    setColour(juce::ListBox::backgroundColourId, p.listBg);
    setColour(juce::ListBox::textColourId, p.listText);
    setColour(juce::FileTreeComponent::backgroundColourId, p.treeBg);
    setColour(juce::FileTreeComponent::linesColourId, p.treeLines);
    setColour(juce::FileTreeComponent::textColourId, p.treeText);

    setColour(meterBackgroundColourId, p.meterBg);
    setColour(meterGradientStartColourId, p.meterStart);
    setColour(meterGradientMidColourId, p.meterMid);
    setColour(meterGradientEndColourId, p.meterEnd);
}

void OrionLookAndFeel::initThemes()
{
    // =================================================================================================
    // LIGHT THEME (Classic)
    // =================================================================================================
    ThemePalette light;
    light.chassis = juce::Colour(0xFFE8E8E8);
    light.panel   = juce::Colour(0xFFDCDCDC);
    light.text    = juce::Colour(0xFF333333);
    light.accent  = juce::Colour(0xFFF07C38);
    light.highlight = juce::Colour(0xFF4CAF50);

    light.dashBg      = juce::Colour(0xFFF2F2F7);
    light.dashSidebar = juce::Colour(0xFFFFFFFF);
    light.dashCard    = juce::Colours::white;
    light.dashText    = juce::Colour(0xFF1D1D1F);
    light.dashAccent  = juce::Colour(0xFF007AFF);

    light.mixerChassis        = juce::Colour(0xFFE0E0E0);
    light.mixerBridge         = juce::Colour(0xFFD8D8D8);
    light.mixerArmRest        = juce::Colour(0xFF383838);
    light.mixerArmRestContour = juce::Colour(0xFF202020);

    light.timelineBg          = juce::Colour(0xFFFAFAFA);
    light.rulerBg             = juce::Colour(0xFFE0E0E0);
    light.grid                = juce::Colour(0xFFE0E0E0);
    light.timelineRecordingRegion = juce::Colour(0xFFFF3030).withAlpha(0.08f);

    light.trackHeaderBg       = juce::Colour(0xFFF5F5F5);
    light.trackHeaderPanel    = juce::Colour(0xFFF5F5F5);
    light.trackHeaderSeparator = juce::Colour(0xFFD0D0D0);

    light.browserBg           = juce::Colour(0xFFE0E0E0);
    light.browserSidebar      = juce::Colour(0xFFF0F0F0);
    light.browserTree         = juce::Colour(0xFFF5F5F5);
    light.browserPreview      = juce::Colour(0xFF151515);

    light.transportBtnBg      = juce::Colour(0xFFE8E8E8);
    light.transportBtnSym     = juce::Colour(0xFF333333);
    light.lcdBg               = juce::Colour(0xFF0D0D0D);
    light.lcdBezel            = juce::Colour(0xFF151515);
    light.lcdText             = juce::Colour(0xFF4AC0FF);
    light.lcdLabel            = juce::Colour(0xFF888888);

    light.toolBtnBg           = juce::Colour(0xFFF0F0F0);
    light.toolBtnActive       = juce::Colour(0xFF505050);

    light.knobBase            = juce::Colour(0xFFE0E0E0);
    light.knobFace            = juce::Colour(0xFFF5F5F5);
    light.knobOutline         = juce::Colour(0xFF888888);

    light.toggleBg            = juce::Colour(0xFFD0D0D0);
    light.toggleOutline       = juce::Colour(0xFF999999);

    light.resizableWindowBg = light.chassis;
    light.documentWindowBg = light.chassis;
    light.textEditorBg = juce::Colours::white;
    light.textEditorText = juce::Colour(0xFF333333);
    light.textEditorHighlight = light.accent.withAlpha(0.3f);
    light.textEditorOutline = juce::Colour(0xFFB0B0B0);
    light.textEditorFocusedOutline = light.accent;
    light.buttonCol = juce::Colour(0xFFF0F0F0);
    light.buttonOnCol = light.highlight;
    light.textButtonTextOff = juce::Colour(0xFF333333);
    light.textButtonTextOn = juce::Colours::white;
    light.sliderThumb = juce::Colour(0xFFF5F5F5);
    light.sliderTrack = juce::Colour(0xFFA0A0A0);
    light.sliderFill = light.accent;
    light.sliderOutline = juce::Colour(0xFF888888);
    light.sliderText = juce::Colour(0xFF333333);
    light.scrollThumb = juce::Colour(0xFF999999);
    light.scrollTrack = light.chassis.darker(0.05f);
    light.tabBg = light.panel;
    light.tabOutline = juce::Colours::transparentBlack;
    light.labelText = juce::Colour(0xFF333333);
    light.toggleText = juce::Colour(0xFF333333);
    light.toggleTick = light.highlight;
    light.groupText = juce::Colour(0xFF333333);
    light.groupOutline = juce::Colour(0xFFB0B0B0);
    light.popupBg = juce::Colours::white.withAlpha(0.95f);
    light.popupText = juce::Colour(0xFF333333);
    light.popupHighlightBg = light.accent;
    light.popupHighlightText = juce::Colours::white;
    light.comboBg = juce::Colours::white;
    light.comboOutline = juce::Colour(0xFFB0B0B0);
    light.comboArrow = juce::Colour(0xFF333333);
    light.comboText = juce::Colour(0xFF333333);
    light.listBg = juce::Colour(0xFFF0F0F0);
    light.listText = juce::Colour(0xFF333333);
    light.treeBg = light.browserTree;
    light.treeLines = juce::Colour(0xFFE0E0E0);
    light.treeText = juce::Colour(0xFF333333);

    light.meterBg = juce::Colour(0xFFE0E0E0);
    light.meterStart = juce::Colour(0xFF00E040);
    light.meterMid = juce::Colour(0xFFFFC000);
    light.meterEnd = juce::Colour(0xFFFF3030);

    themes["Light"] = light;

    // =================================================================================================
    // DARK THEME (Classic)
    // =================================================================================================
    ThemePalette dark;
    dark.chassis = juce::Colour(0xFF181818);
    dark.panel   = juce::Colour(0xFF242424);
    dark.text    = juce::Colours::white;
    dark.accent  = juce::Colour(0xFF3498DB);
    dark.highlight = juce::Colour(0xFF2ECC71);

    dark.dashBg      = juce::Colour(0xFF121212);
    dark.dashSidebar = juce::Colour(0xFF1E1E1E);
    dark.dashCard    = juce::Colour(0xFF242424);
    dark.dashText    = juce::Colours::white;
    dark.dashAccent  = juce::Colour(0xFF3498DB);

    dark.mixerChassis        = juce::Colour(0xFF181818);
    dark.mixerBridge         = juce::Colour(0xFF202020);
    dark.mixerArmRest        = juce::Colour(0xFF141414);
    dark.mixerArmRestContour = juce::Colour(0xFF0A0A0A);

    dark.timelineBg          = juce::Colour(0xFF1E1E1E);
    dark.rulerBg             = juce::Colour(0xFF252525);
    dark.grid                = juce::Colour(0xFF333333);
    dark.timelineRecordingRegion = juce::Colour(0xFFE74C3C).withAlpha(0.15f);

    dark.trackHeaderBg       = juce::Colour(0xFF252525);
    dark.trackHeaderPanel    = juce::Colour(0xFF252525);
    dark.trackHeaderSeparator = juce::Colour(0xFF121212);

    dark.browserBg           = juce::Colour(0xFF181818);
    dark.browserSidebar      = juce::Colour(0xFF202020);
    dark.browserTree         = juce::Colour(0xFF181818);
    dark.browserPreview      = juce::Colour(0xFF141414);

    dark.transportBtnBg      = juce::Colour(0xFF2C2C2C);
    dark.transportBtnSym     = juce::Colours::white;
    dark.lcdBg               = juce::Colour(0xFF0A0A0A);
    dark.lcdBezel            = juce::Colour(0xFF2C2C2C);
    dark.lcdText             = juce::Colour(0xFF64D2FF);
    dark.lcdLabel            = juce::Colours::white;

    dark.toolBtnBg           = juce::Colour(0xFF2C2C2C);
    dark.toolBtnActive       = juce::Colour(0xFF444444);

    dark.knobBase            = juce::Colour(0xFF282828);
    dark.knobFace            = juce::Colour(0xFF383838);
    dark.knobOutline         = juce::Colour(0xFF101010);

    dark.toggleBg            = juce::Colour(0xFF101010);
    dark.toggleOutline       = juce::Colour(0xFF000000);

    dark.resizableWindowBg = dark.chassis;
    dark.documentWindowBg = dark.chassis;
    dark.textEditorBg = juce::Colour(0xFF121212);
    dark.textEditorText = juce::Colours::white;
    dark.textEditorHighlight = dark.accent.withAlpha(0.3f);
    dark.textEditorOutline = juce::Colour(0xFF383838);
    dark.textEditorFocusedOutline = dark.accent;
    dark.buttonCol = juce::Colour(0xFF303030);
    dark.buttonOnCol = dark.highlight;
    dark.textButtonTextOff = juce::Colours::white;
    dark.textButtonTextOn = juce::Colours::white;
    dark.sliderThumb = juce::Colour(0xFF555555);
    dark.sliderTrack = juce::Colour(0xFF222222);
    dark.sliderFill = dark.accent;
    dark.sliderOutline = juce::Colour(0xFF666666);
    dark.sliderText = juce::Colours::white;
    dark.scrollThumb = juce::Colour(0xFF666666);
    dark.scrollTrack = dark.chassis.darker(0.05f);
    dark.tabBg = dark.panel;
    dark.tabOutline = juce::Colours::transparentBlack;
    dark.labelText = juce::Colours::white;
    dark.toggleText = juce::Colours::white;
    dark.toggleTick = dark.highlight;
    dark.groupText = juce::Colours::white;
    dark.groupOutline = juce::Colour(0xFF444444);
    dark.popupBg = juce::Colour(0xFF222222).withAlpha(0.95f);
    dark.popupText = juce::Colours::white;
    dark.popupHighlightBg = dark.accent;
    dark.popupHighlightText = juce::Colours::white;
    dark.comboBg = juce::Colour(0xFF151515);
    dark.comboOutline = juce::Colour(0xFF444444);
    dark.comboArrow = juce::Colours::white;
    dark.comboText = juce::Colours::white;
    dark.listBg = juce::Colour(0xFF1A1A1A);
    dark.listText = juce::Colours::white;
    dark.treeBg = dark.browserTree;
    dark.treeLines = juce::Colour(0xFF333333);
    dark.treeText = juce::Colours::white;

    dark.meterBg = juce::Colour(0xFF101010);
    dark.meterStart = juce::Colour(0xFF00E040);
    dark.meterMid = juce::Colour(0xFFFFC000);
    dark.meterEnd = juce::Colour(0xFFFF3030);

    themes["Dark"] = dark;

    // =================================================================================================
    // DEEP SPACE (Professional Dark)
    // Inspired by Studio One and Logic Pro - Refined, high contrast, subtle blues
    // =================================================================================================
    ThemePalette deepSpace = dark;
    juce::Colour spaceBlack = juce::Colour(0xFF0E1116); // Deep charcoal
    juce::Colour spaceBlue  = juce::Colour(0xFF1A1F26); // Modern slate
    juce::Colour accentBlue = juce::Colour(0xFF3B82F6); // Professional vibrant blue
    juce::Colour accentCyan = juce::Colour(0xFF06B6D4);

    deepSpace.chassis = spaceBlack;
    deepSpace.panel = spaceBlue;
    deepSpace.text = juce::Colours::white;
    deepSpace.dashBg = juce::Colour(0xFF080A0F);
    deepSpace.dashSidebar = spaceBlack;
    deepSpace.dashCard = spaceBlue.brighter(0.02f);
    deepSpace.dashText = juce::Colours::white;
    deepSpace.dashAccent = accentBlue;
    deepSpace.accent = accentBlue;
    deepSpace.highlight = accentCyan;

    deepSpace.mixerChassis = spaceBlack;
    deepSpace.mixerBridge = spaceBlue;
    deepSpace.mixerArmRest = spaceBlack.darker(0.2f);

    deepSpace.timelineBg = spaceBlue.darker(0.05f);
    deepSpace.rulerBg = spaceBlue.brighter(0.05f);
    deepSpace.trackHeaderBg = spaceBlue;
    deepSpace.trackHeaderPanel = spaceBlue;

    deepSpace.lcdBg = juce::Colours::black;
    deepSpace.lcdText = accentCyan;
    deepSpace.sliderFill = accentBlue;
    deepSpace.buttonOnCol = accentBlue;
    deepSpace.popupHighlightBg = accentBlue;

    deepSpace.textEditorBg = spaceBlack.darker(0.1f);
    deepSpace.textEditorOutline = spaceBlue.brighter(0.1f);
    deepSpace.textEditorFocusedOutline = accentBlue;

    deepSpace.meterBg = spaceBlack;
    deepSpace.meterStart = juce::Colour(0xFF10B981); // Emerald
    deepSpace.meterMid = juce::Colour(0xFFF59E0B);   // Amber
    deepSpace.meterEnd = juce::Colour(0xFFEF4444);   // Rose

    themes["Deep Space"] = deepSpace;

    // =================================================================================================
    // NEBULA
    // Deep purple, magenta, cosmic vibes
    // =================================================================================================
    ThemePalette nebula = dark;
    juce::Colour cosmicDark = juce::Colour(0xFF1A0B1F);
    juce::Colour cosmicPanel = juce::Colour(0xFF26142E);
    juce::Colour magenta = juce::Colour(0xFFFF00CC);
    juce::Colour violet = juce::Colour(0xFF9B59B6);

    nebula.chassis = cosmicDark;
    nebula.panel = cosmicPanel;
    nebula.dashBg = cosmicDark.darker(0.2f);
    nebula.dashSidebar = cosmicDark;
    nebula.dashCard = cosmicPanel;
    nebula.accent = magenta;
    nebula.highlight = violet;

    nebula.mixerChassis = cosmicDark;
    nebula.mixerBridge = cosmicPanel;
    nebula.timelineBg = cosmicPanel;
    nebula.trackHeaderBg = cosmicPanel.darker(0.1f);

    nebula.sliderFill = magenta;
    nebula.lcdText = magenta.brighter(0.2f);
    nebula.popupHighlightBg = magenta;
    nebula.dashAccent = magenta;

    nebula.meterBg = cosmicDark.brighter(0.05f);
    nebula.meterStart = violet.withAlpha(0.6f);
    nebula.meterMid = violet;
    nebula.meterEnd = magenta;

    themes["Nebula"] = nebula;

    // =================================================================================================
    // STARLIGHT
    // Modern Light, Silver, Clean, Professional
    // =================================================================================================
    ThemePalette starlight = light;
    juce::Colour silver = juce::Colour(0xFFF0F2F5);
    juce::Colour chrome = juce::Colour(0xFFFFFFFF);
    juce::Colour proBlue = juce::Colour(0xFF007AFF);

    starlight.chassis = silver;
    starlight.panel = chrome;
    starlight.dashBg = juce::Colour(0xFFF5F7FA);
    starlight.dashSidebar = chrome;
    starlight.accent = proBlue;
    starlight.highlight = proBlue;

    starlight.mixerChassis = silver;
    starlight.mixerBridge = chrome;
    starlight.timelineBg = chrome;
    starlight.trackHeaderBg = juce::Colour(0xFFECEEF0);

    starlight.sliderFill = proBlue;
    starlight.lcdText = proBlue;
    starlight.popupHighlightBg = proBlue;
    starlight.dashAccent = proBlue;

    starlight.meterBg = chrome.darker(0.05f);
    starlight.meterStart = proBlue.withAlpha(0.5f);
    starlight.meterMid = proBlue;
    starlight.meterEnd = juce::Colour(0xFF000080); // Dark Blue

    themes["Starlight"] = starlight;

    // =================================================================================================
    // RED GIANT
    // Dark, Warm, Red/Orange/Amber
    // =================================================================================================
    ThemePalette redGiant = dark;
    juce::Colour deepRed = juce::Colour(0xFF1F0B0B);
    juce::Colour warmGrey = juce::Colour(0xFF2E1C1C);
    juce::Colour amber = juce::Colour(0xFFFF8C00);
    juce::Colour heatRed = juce::Colour(0xFFFF3030);

    redGiant.chassis = deepRed;
    redGiant.panel = warmGrey;
    redGiant.dashBg = deepRed.darker(0.1f);
    redGiant.dashSidebar = deepRed;
    redGiant.dashCard = warmGrey;
    redGiant.accent = amber;
    redGiant.highlight = heatRed;

    redGiant.mixerChassis = deepRed;
    redGiant.mixerBridge = warmGrey;
    redGiant.timelineBg = warmGrey;
    redGiant.trackHeaderBg = warmGrey.darker(0.1f);

    redGiant.sliderFill = amber;
    redGiant.lcdText = amber;
    redGiant.popupHighlightBg = amber;
    redGiant.dashAccent = amber;

    redGiant.meterBg = deepRed.brighter(0.05f);
    redGiant.meterStart = amber.darker(0.2f);
    redGiant.meterMid = amber;
    redGiant.meterEnd = heatRed;

    themes["Red Giant"] = redGiant;

    // =================================================================================================
    // VINTAGE
    // Cream, Beige, Brown, Analog gear, Wood hints
    // =================================================================================================
    ThemePalette vintage;
    juce::Colour cream = juce::Colour(0xFFE8E4D9);
    juce::Colour beige = juce::Colour(0xFFDCD6C5);
    juce::Colour wood = juce::Colour(0xFF5D4037);
    juce::Colour darkWood = juce::Colour(0xFF3E2723);
    juce::Colour brass = juce::Colour(0xFFBCAAA4);
    juce::Colour ink = juce::Colour(0xFF3E2723);
    juce::Colour vuWarm = juce::Colour(0xFFFFE0B2);

    vintage = light; // Start with light base

    vintage.chassis = beige;
    vintage.panel = cream;
    vintage.text = ink;
    vintage.accent = wood;
    vintage.highlight = juce::Colour(0xFFD84315); // Burnt orange

    vintage.dashBg = cream.darker(0.05f);
    vintage.dashSidebar = beige;
    vintage.dashCard = cream;
    vintage.dashText = ink;

    vintage.mixerChassis = beige;
    vintage.mixerBridge = cream;
    vintage.mixerArmRest = wood;
    vintage.mixerArmRestContour = darkWood;

    vintage.timelineBg = cream;
    vintage.rulerBg = beige;
    vintage.grid = juce::Colour(0xFFD7CCC8);
    vintage.trackHeaderBg = beige;
    vintage.trackHeaderPanel = cream;

    vintage.transportBtnBg = beige.darker(0.05f);
    vintage.transportBtnSym = ink;
    vintage.lcdBg = juce::Colour(0xFF2C2520);
    vintage.lcdText = vuWarm;
    vintage.lcdBezel = darkWood;

    vintage.knobBase = juce::Colour(0xFFD7CCC8);
    vintage.knobFace = cream;
    vintage.knobOutline = wood;

    vintage.sliderFill = wood;
    vintage.sliderThumb = wood;
    vintage.textEditorBg = cream.brighter(0.1f);
    vintage.textEditorText = ink;
    vintage.popupBg = cream;
    vintage.popupText = ink;
    vintage.popupHighlightBg = wood;
    vintage.popupHighlightText = cream;

    vintage.meterBg = cream.darker(0.1f);
    vintage.meterStart = vuWarm.darker(0.1f);
    vintage.meterMid = vuWarm;
    vintage.meterEnd = juce::Colour(0xFFD84315);

    themes["Vintage"] = vintage;

    // =================================================================================================
    // CYBERPUNK
    // High contrast, Black, Neon Green, Neon Pink, Glitchy
    // =================================================================================================
    ThemePalette cyberpunk = dark;
    juce::Colour cyberBlack = juce::Colours::black;
    juce::Colour cyberGrey = juce::Colour(0xFF111111);
    juce::Colour neonGreen = juce::Colour(0xFF39FF14);
    juce::Colour neonPink = juce::Colour(0xFFFF00FF);

    cyberpunk.chassis = cyberBlack;
    cyberpunk.panel = cyberGrey;
    cyberpunk.accent = neonGreen;
    cyberpunk.highlight = neonPink;

    cyberpunk.dashBg = cyberBlack;
    cyberpunk.dashSidebar = cyberGrey;
    cyberpunk.dashCard = juce::Colour(0xFF1A1A1A);
    cyberpunk.dashAccent = neonGreen;

    cyberpunk.mixerChassis = cyberBlack;
    cyberpunk.mixerBridge = cyberGrey;
    cyberpunk.mixerArmRest = juce::Colour(0xFF050505);

    cyberpunk.timelineBg = cyberGrey;
    cyberpunk.grid = juce::Colour(0xFF333333);
    cyberpunk.trackHeaderBg = cyberGrey;

    cyberpunk.sliderFill = neonGreen;
    cyberpunk.lcdText = neonGreen;
    cyberpunk.popupHighlightBg = neonGreen;
    cyberpunk.popupHighlightText = cyberBlack;
    cyberpunk.textEditorHighlight = neonPink.withAlpha(0.4f);
    cyberpunk.textEditorFocusedOutline = neonGreen;

    cyberpunk.meterBg = cyberGrey;
    cyberpunk.meterStart = neonGreen.withAlpha(0.5f);
    cyberpunk.meterMid = neonGreen;
    cyberpunk.meterEnd = neonPink;

    themes["Cyberpunk"] = cyberpunk;

    // =================================================================================================
    // MIDNIGHT VOID
    // Ultra Dark, Monochrome-ish
    // =================================================================================================
    ThemePalette voidTheme = dark;
    juce::Colour voidBlack = juce::Colour(0xFF000000);
    juce::Colour voidGrey = juce::Colour(0xFF0A0A0A);
    juce::Colour whiteGhost = juce::Colour(0xFFDDDDDD);

    voidTheme.chassis = voidBlack;
    voidTheme.panel = voidGrey;
    voidTheme.accent = whiteGhost;
    voidTheme.highlight = juce::Colours::white;

    voidTheme.dashBg = voidBlack;
    voidTheme.dashSidebar = voidBlack;
    voidTheme.dashCard = voidGrey;
    voidTheme.dashAccent = whiteGhost;

    voidTheme.mixerChassis = voidBlack;
    voidTheme.mixerBridge = voidGrey;
    voidTheme.timelineBg = voidGrey;
    voidTheme.trackHeaderBg = voidGrey;

    voidTheme.sliderFill = whiteGhost;
    voidTheme.lcdText = whiteGhost;
    voidTheme.popupHighlightBg = whiteGhost;
    voidTheme.popupHighlightText = voidBlack;

    voidTheme.meterBg = voidBlack.brighter(0.05f);
    voidTheme.meterStart = juce::Colours::white.withAlpha(0.3f);
    voidTheme.meterMid = juce::Colours::white.withAlpha(0.6f);
    voidTheme.meterEnd = juce::Colours::white;

    themes["Midnight Void"] = voidTheme;
}


void OrionLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         const float rotaryStartAngle, const float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto radius = (float) juce::jmin (width / 2, height / 2) - 6.0f;
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);


    {
        juce::Path shadowPath;
        shadowPath.addEllipse(centreX - radius - 2, centreY - radius - 2, (radius + 2) * 2.0f, (radius + 2) * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillPath(shadowPath);


        juce::Path dropShadow;
        dropShadow.addEllipse(centreX - radius + 2, centreY - radius + 4.0f, radius * 2.0f, radius * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillPath(dropShadow);
    }

    juce::Colour base = findColour(knobBaseColourId);
    juce::Colour gradTop = base.brighter(0.1f);
    juce::Colour gradBot = base.darker(0.3f);

    juce::ColourGradient bodyGrad(gradTop, centreX, centreY - radius,
                                  gradBot, centreX, centreY + radius, false);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(findColour(knobOutlineColourId));
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    float faceIndent = 4.0f;
    float faceRadius = radius - faceIndent;

    juce::Colour faceBase = findColour(knobFaceColourId);
    juce::ColourGradient faceGrad(faceBase.brighter(0.2f), centreX - faceRadius * 0.5f, centreY - faceRadius * 0.5f,
                                  faceBase.darker(0.1f), centreX + faceRadius, centreY + faceRadius, true);

    g.setGradientFill(faceGrad);
    g.fillEllipse(centreX - faceRadius, centreY - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);


    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawEllipse(centreX - faceRadius * 0.7f, centreY - faceRadius * 0.7f, faceRadius * 1.4f, faceRadius * 1.4f, 1.0f);
    g.drawEllipse(centreX - faceRadius * 0.4f, centreY - faceRadius * 0.4f, faceRadius * 0.8f, faceRadius * 0.8f, 1.0f);

    // Dynamic reflection opacity
    bool isDark = (findColour(juce::ResizableWindow::backgroundColourId).getBrightness() < 0.5f);
    g.setColour(isDark ? juce::Colours::white.withAlpha(0.1f) : juce::Colours::white.withAlpha(0.5f));

    g.drawEllipse(centreX - faceRadius, centreY - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f, 1.0f);

    juce::Path p;
    float pointerLen = faceRadius * 0.6f;
    float pointerThick = 3.0f;
    p.addRoundedRectangle(-pointerThick * 0.5f, -faceRadius + 4.0f, pointerThick, pointerLen, 1.0f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillPath(p);

    juce::Colour pointerCol = findColour(juce::Slider::rotarySliderFillColourId);
    if (!slider.isEnabled()) pointerCol = pointerCol.withSaturation(pointerCol.getSaturation() * 0.5f).withAlpha(0.5f);

    juce::Path pFill;
    pFill.addRoundedRectangle(-1.0f, -faceRadius + 5.0f, 2.0f, pointerLen - 2.0f, 1.0f);
    pFill.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(pointerCol);
    g.fillPath(pFill);


    {
        juce::Path glass;
        glass.addCentredArc(centreX, centreY, faceRadius * 0.9f, faceRadius * 0.8f, 0.0f, -0.5f, 0.5f, true);

        juce::ColourGradient gloss(juce::Colours::white.withAlpha(0.2f), centreX, centreY - faceRadius,
                                   juce::Colours::transparentBlack, centreX, centreY, false);
        g.setGradientFill(gloss);
        g.fillPath(glass);
    }


    if (slider.isEnabled())
    {
        juce::Path valuePath;

        float ringRadius = radius + 4.0f;


        juce::Path bgPath;
        bgPath.addCentredArc(centreX, centreY, ringRadius, ringRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(findColour(juce::Slider::trackColourId).withAlpha(0.4f));
        g.strokePath(bgPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));


        double min = slider.getMinimum();
        double max = slider.getMaximum();

        float angleStart = rotaryStartAngle;
        float angleEnd = angle;

        if (min < 0.0 && max > 0.0)
        {
            float zeroPos = (float)((0.0 - min) / (max - min));
            float zeroAngle = rotaryStartAngle + zeroPos * (rotaryEndAngle - rotaryStartAngle);


            angleStart = juce::jmin(zeroAngle, angle);
            angleEnd = juce::jmax(zeroAngle, angle);
        }
        else
        {

             angleStart = juce::jmin(rotaryStartAngle, angle);
             angleEnd = juce::jmax(rotaryStartAngle, angle);
        }

        valuePath.addCentredArc(centreX, centreY, ringRadius, ringRadius, 0.0f, angleStart, angleEnd, true);

        g.setColour(pointerCol.withAlpha(0.8f));
        g.strokePath(valuePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void OrionLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos);

    if (style == juce::Slider::LinearVertical)
    {

        float trackWidth = 8.0f;
        float trackX = (float)x + (float)width * 0.5f - trackWidth * 0.5f;
        float trackH = (float)height;


        g.setColour(juce::Colour(0xFF101010));
        g.fillRoundedRectangle(trackX, (float)y, trackWidth, trackH, trackWidth * 0.5f);


        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.drawRoundedRectangle(trackX, (float)y, trackWidth, trackH, trackWidth * 0.5f, 1.0f);


        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawVerticalLine((int)(trackX + trackWidth * 0.5f), (float)y + 4.0f, (float)y + trackH - 4.0f);


        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.2f));
        int numTicks = 10;
        float tickX_L = trackX - 8.0f;
        float tickX_R = trackX + trackWidth + 8.0f;

        for (int i=0; i<=numTicks; ++i) {
            float tickY = (float)y + ((float)i / numTicks) * trackH;


            float len = (i == 0 || i == numTicks || i == numTicks/2) ? 6.0f : 3.0f;

            g.drawLine(tickX_L - len, tickY, tickX_L, tickY, 1.0f);
            g.drawLine(tickX_R, tickY, tickX_R + len, tickY, 1.0f);
        }


        float thumbWidth = 36.0f;
        float thumbHeight = 54.0f;
        float thumbX = (float)x + (float)width * 0.5f - thumbWidth * 0.5f;
        float thumbY = sliderPos - (thumbHeight * 0.5f);

        juce::Rectangle<float> thumbRect(thumbX, thumbY, thumbWidth, thumbHeight);


        {
            juce::Path shadow;
            shadow.addRoundedRectangle(thumbRect.translated(0, 5), 3.0f);
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillPath(shadow);
        }


        juce::Colour capBase = findColour(knobBaseColourId);
        if (slider.isMouseOverOrDragging()) capBase = capBase.brighter(0.05f);


        juce::ColourGradient capGrad(capBase.brighter(0.15f), thumbX, thumbY,
                                     capBase.darker(0.3f), thumbX, thumbY + thumbHeight, false);
        capGrad.addColour(0.48, capBase);
        capGrad.addColour(0.52, capBase.darker(0.1f));

        g.setGradientFill(capGrad);
        g.fillRoundedRectangle(thumbRect, 3.0f);


        juce::ColourGradient gloss(juce::Colours::white.withAlpha(0.15f), thumbX, thumbY,
                                   juce::Colours::transparentBlack, thumbX, thumbY + thumbHeight * 0.4f, false);
        g.setGradientFill(gloss);
        g.fillRoundedRectangle(thumbRect.reduced(1.0f), 3.0f);


        float lineY = thumbY + thumbHeight * 0.5f;
        float lineH = 2.0f;


        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRect(thumbX + 3.0f, lineY - 1.0f, thumbWidth - 6.0f, lineH);


        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawLine(thumbX + 3.0f, lineY + 1.5f, thumbX + thumbWidth - 3.0f, lineY + 1.5f);


        int numRidges = 5;
        float spacing = 4.0f;

        for (int i = -numRidges; i <= numRidges; ++i) {
            if (i == 0) continue;
            float ry = lineY + (float)i * spacing;


            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.drawLine(thumbX + 6.0f, ry, thumbX + thumbWidth - 6.0f, ry, 1.0f);


            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawLine(thumbX + 6.0f, ry + 1.0f, thumbX + thumbWidth - 6.0f, ry + 1.0f, 1.0f);
        }


        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(thumbRect, 3.0f, 1.0f);
    }
    else
    {


        float trackH = 4.0f;
        float trackY = (float)y + (float)height * 0.5f - trackH * 0.5f;

        g.setColour(findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle((float)x, trackY, (float)width, trackH, 2.0f);


        if (minSliderPos != maxSliderPos)
        {


        }


        float r = (float)height * 0.4f;
        float handleX = sliderPos;

        juce::Colour handleCol = findColour(juce::Slider::thumbColourId);
        g.setColour(handleCol);
        g.fillEllipse(handleX - r, (float)y + (float)height * 0.5f - r, r*2, r*2);


        juce::ColourGradient hg(juce::Colours::white.withAlpha(0.3f), handleX - r, (float)y,
                                juce::Colours::transparentBlack, handleX, (float)y + height, false);
        g.setGradientFill(hg);
        g.fillEllipse(handleX - r, (float)y + (float)height * 0.5f - r, r*2, r*2);
    }
}



void OrionLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    float cornerSize = 3.0f;

    bool isDown = shouldDrawButtonAsDown;
    bool isToggled = button.getToggleState();
    juce::Colour baseColour = backgroundColour;

    if (button.getClickingTogglesState() && isToggled) {
         juce::Colour onCol = button.findColour(juce::TextButton::buttonOnColourId);
         if (!onCol.isTransparent()) baseColour = onCol;
    }

    // Realistic Lighting
    if (isToggled || isDown) {
        juce::ColourGradient grad(baseColour.darker(0.15f), 0, bounds.getY(),
                                  baseColour.brighter(0.05f), 0, bounds.getBottom(), false);
        g.setGradientFill(grad);
    } else {
        juce::ColourGradient grad(baseColour.brighter(0.05f), 0, bounds.getY(),
                                  baseColour.darker(0.15f), 0, bounds.getBottom(), false);
        g.setGradientFill(grad);
    }
    g.fillRoundedRectangle(bounds, cornerSize);

    // Inner glow for "On" state
    if (isToggled) {
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, 1.2f);

        // Subtle bloom
        g.setColour(baseColour.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, cornerSize, 2.0f);
    }

    // Modern Border
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    if (shouldDrawButtonAsHighlighted && !isDown)
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
}

void OrionLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    auto fontSize = jmin (15.0f, (float) button.getHeight() * 0.75f);
    auto tickWidth = fontSize * 1.1f;

    auto tickRect = juce::Rectangle<float>(4.0f, ((float) button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth);

    g.setColour(findColour(toggleSwitchBackgroundColourId));
    g.fillRoundedRectangle(tickRect, 3.0f);

    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawRoundedRectangle(tickRect, 3.0f, 1.0f);

    if (button.getToggleState()) {
        juce::Colour onCol = button.findColour(juce::ToggleButton::tickColourId);
        if (onCol.isTransparent()) onCol = findColour(juce::TextButton::buttonOnColourId);

        g.setColour(onCol);
        g.fillRoundedRectangle(tickRect.reduced(3.0f), 2.0f);

        // Simple glow
        g.setColour(onCol.withAlpha(0.4f));
        g.drawRoundedRectangle(tickRect.reduced(3.0f), 2.0f, 2.0f);
    }

    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (fontSize);

    if (! button.getButtonText().isEmpty())
        g.drawFittedText (button.getButtonText(),
                          (int) (tickWidth + 10.0f), 0,
                          button.getWidth() - (int) (tickWidth + 10.0f), button.getHeight(),
                          juce::Justification::centredLeft, 2);
}



void OrionLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH, isButtonDown);

    auto cornerSize = 4.0f;
    juce::Rectangle<int> boxBounds (0, 0, width, height);


    juce::Colour bg = box.findColour (juce::ComboBox::backgroundColourId);


    juce::ColourGradient grad(bg.brighter(0.05f), 0, 0,
                              bg.darker(0.1f), 0, (float)height, false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);


    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawLine(0, 0, (float)width, 0, 1.0f);


    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);


    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawVerticalLine(width - 24, 2.0f, (float)height - 2.0f);


    juce::Rectangle<int> arrowZone (width - 24, 0, 20, height);
    juce::Path path;
    path.startNewSubPath ((float) arrowZone.getX() + 8.0f, (float) arrowZone.getCentreY() - 2.0f);
    path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
    path.lineTo ((float) arrowZone.getRight() - 8.0f, (float) arrowZone.getCentreY() - 2.0f);

    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 1.0f : 0.5f)));
    g.strokePath (path, juce::PathStrokeType (1.5f));
}

void OrionLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getLabelFont (label));
}



void OrionLookAndFeel::preparePopupMenuWindow (juce::Component& window)
{
    window.setOpaque (false);
}

void OrionLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto bg = findColour (juce::PopupMenu::backgroundColourId);
    auto area = juce::Rectangle<int> (width, height).toFloat();

    g.setColour (bg.withAlpha (0.9f));
    g.fillRoundedRectangle (area, 6.0f);

    bool isDark = bg.getBrightness() < 0.5f;
    g.setColour (isDark ? bg.brighter (0.12f) : bg.darker (0.12f));
    g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);
}

void OrionLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                                          const juce::String& text, const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    juce::ignoreUnused (textColourToUse);

    if (isSeparator)
    {
        auto r = area.reduced (5, 0);
        g.setColour (findColour (juce::PopupMenu::textColourId).withAlpha (0.1f));
        g.drawHorizontalLine (r.getY() + r.getHeight() / 2, (float) r.getX(), (float) r.getRight());
        return;
    }

    auto r = area.toFloat();

    if (isHighlighted && isActive)
    {
        g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId).withAlpha (0.85f));
        g.fillRoundedRectangle (r.reduced (2.0f, 1.0f), 4.0f);
    }

    g.setColour (findColour (isHighlighted ? juce::PopupMenu::highlightedTextColourId : juce::PopupMenu::textColourId));

    if (!isActive)
        g.setOpacity (0.4f);

    auto font = getPopupMenuFont();
    auto maxFontHeight = r.getHeight() * 0.6f;

    if (font.getHeight() > maxFontHeight)
        font.setHeight (maxFontHeight);

    g.setFont (font);

    auto iconArea = r.removeFromLeft (r.getHeight());

    if (icon != nullptr)
    {
        icon->drawWithin (g, iconArea.reduced (r.getHeight() * 0.2f), juce::RectanglePlacement::centred, 1.0f);
    }
    else if (isTicked)
    {
        auto tick = getTickShape (2.0f);
        g.fillPath (tick, tick.getTransformToScaleToFit (iconArea.reduced (r.getHeight() * 0.3f), true));
    }

    if (hasSubMenu)
    {
        auto arrowArea = r.removeFromRight (r.getHeight());
        auto arrowH = arrowArea.getHeight() * 0.3f;
        auto x = arrowArea.getX() + (arrowArea.getWidth() - arrowH) * 0.5f;
        auto y = arrowArea.getY() + (arrowArea.getHeight() - arrowH) * 0.5f;

        juce::Path path;
        path.startNewSubPath (x, y);
        path.lineTo (x + arrowH, y + arrowH * 0.5f);
        path.lineTo (x, y + arrowH);
        path.closeSubPath();

        g.fillPath (path);
    }

    r.removeFromRight (3);
    g.drawFittedText (text, r.toNearestInt(), juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty())
    {
        auto font2 = font;
        font2.setHeight (font.getHeight() * 0.75f);
        font2.setHorizontalScale (0.9f);
        g.setFont (font2);

        g.drawText (shortcutKeyText, r.toNearestInt(), juce::Justification::centredRight, true);
    }
}

juce::Font OrionLookAndFeel::getPopupMenuFont()
{
    return Orion::UI::DesignSystem::instance().fonts().sans(15.0f, juce::Font::plain);
}



void OrionLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                              juce::TextEditor& textEditor)
{
    if (textEditor.isEnabled())
    {
        if (textEditor.hasKeyboardFocus (true) && ! textEditor.isReadOnly())
        {
            g.setColour (textEditor.findColour (juce::TextEditor::focusedOutlineColourId));
            g.drawRect (0, 0, width, height, 1);
        }
        else
        {
            g.setColour (textEditor.findColour (juce::TextEditor::outlineColourId));
            g.drawRect (0, 0, width, height);
        }
    }
}



void OrionLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
                                      bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                      bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(scrollbar, isMouseOver, isMouseDown);


    g.setColour (findColour(juce::ScrollBar::trackColourId));
    g.fillRect (x, y, width, height);


    g.setColour (findColour(juce::ScrollBar::thumbColourId));

    if (isScrollbarVertical)
    {
        g.fillRoundedRectangle((float)x + 2, (float)thumbStartPosition, (float)width - 4, (float)thumbSize, 3.0f);
    }
    else
    {
        g.fillRoundedRectangle((float)thumbStartPosition, (float)y + 2, (float)thumbSize, (float)height - 4, 3.0f);
    }
}



juce::Font OrionLookAndFeel::getLabelFont (juce::Label&)
{
    return Orion::UI::DesignSystem::instance().fonts().sans(14.0f, juce::Font::plain);
}



void OrionLookAndFeel::drawMenuBarBackground (juce::Graphics& , int , int ,
                                              bool, juce::MenuBarComponent&)
{

}

int OrionLookAndFeel::getMenuBarItemWidth (juce::MenuBarComponent&, int, const juce::String& itemText)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (juce::Font (juce::FontOptions (15.0f)), itemText, 0.0f, 0.0f);
    return (int) ga.getBoundingBox (0, -1, true).getWidth() + 24;
}

juce::Font OrionLookAndFeel::getMenuBarFont (juce::MenuBarComponent&, int, const juce::String&)
{
    return juce::Font (juce::FontOptions(15.0f));
}

void OrionLookAndFeel::drawMenuBarItem (juce::Graphics& g, int width, int height,
                                        int, const juce::String& itemText,
                                        bool isMouseOverItem, bool isMenuOpen,
                                        bool, juce::MenuBarComponent& menuBar)
{
    juce::ignoreUnused(menuBar);

    if (isMouseOverItem || isMenuOpen)
    {
        juce::Colour bg = findColour(juce::PopupMenu::backgroundColourId);
        bool isDark = bg.getBrightness() < 0.5f;
        g.setColour (isDark ? bg.brighter(0.1f) : bg.darker(0.1f));
        g.fillRect (0, 0, width, height);
    }


    g.setColour (findColour(juce::PopupMenu::textColourId));
    g.setFont (15.0f);
    g.drawFittedText (itemText, 0, 0, width, height, juce::Justification::centred, 1);
}
