#include "OrionDesignSystem.h"

#include <array>

namespace Orion::UI {
namespace {

juce::File findAssetPath(const juce::String& relativePath)
{
    juce::Array<juce::File> roots;
    roots.add(juce::File::getCurrentWorkingDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getParentDirectory());

    for (const auto& root : roots)
    {
        auto candidate = root.getChildFile(relativePath);
        if (candidate.existsAsFile())
            return candidate;
    }

    return {};
}

juce::Typeface::Ptr loadTypefaceIfAvailable(const juce::String& relativePath)
{
    auto file = findAssetPath(relativePath);
    if (!file.existsAsFile())
        return {};

    auto stream = file.createInputStream();
    if (stream == nullptr)
        return {};

    juce::MemoryBlock block;
    stream->readIntoMemoryBlock(block);
    if (block.getSize() == 0)
        return {};

    return juce::Typeface::createSystemTypefaceFor(block.getData(), block.getSize());
}

ThemeTokens makeTheme(const ThemeDescriptor& descriptor)
{
    ThemeTokens theme;
    theme.id = descriptor.id;
    theme.displayName = descriptor.displayName;
    theme.light = descriptor.isLight;

    if (descriptor.id == "studio_one_dark")
    {
        theme.colors = {
            juce::Colour(0xFF0D1117), // appBg
            juce::Colour(0xFF161B22), // panelBg
            juce::Colour(0xFF1F2733), // panelBgAlt
            juce::Colour(0xFF11161E), // chromeBg
            juce::Colour(0xFF2D3746), // border
            juce::Colour(0xFFE7EDF8), // textPrimary
            juce::Colour(0xFF9BA8BE), // textSecondary
            juce::Colour(0xFF3B82F6), // accent
            juce::Colour(0xFF60A5FA), // accentStrong
            juce::Colour(0xFFEF4444), // danger
            juce::Colour(0xFF10B981)  // success
        };
        return theme;
    }

    if (descriptor.isLight)
    {
        theme.colors = {
            juce::Colour(0xFFF3F5F8),
            juce::Colour(0xFFFFFFFF),
            juce::Colour(0xFFE9EEF6),
            juce::Colour(0xFFF8FAFC),
            juce::Colour(0xFFD2D9E4),
            juce::Colour(0xFF162030),
            juce::Colour(0xFF4A5A73),
            juce::Colour(0xFF2563EB),
            juce::Colour(0xFF1D4ED8),
            juce::Colour(0xFFDC2626),
            juce::Colour(0xFF059669)
        };
        return theme;
    }

    // Dark fallback family.
    theme.colors = {
        juce::Colour(0xFF12151A),
        juce::Colour(0xFF1B222C),
        juce::Colour(0xFF252D3A),
        juce::Colour(0xFF141A22),
        juce::Colour(0xFF323C4C),
        juce::Colour(0xFFEAF0FA),
        juce::Colour(0xFFAAB5C8),
        juce::Colour(0xFF4B8BFF),
        juce::Colour(0xFF69A1FF),
        juce::Colour(0xFFFB7185),
        juce::Colour(0xFF34D399)
    };

    return theme;
}

} // namespace

FontPack::FontPack()
{
    // OFL fonts shipped in assets/fonts with runtime fallback to system fonts.
    sansRegular = loadTypefaceIfAvailable("assets/fonts/Inter-Regular.ttf");
    sansBold = loadTypefaceIfAvailable("assets/fonts/Inter-SemiBold.ttf");
    monoRegular = loadTypefaceIfAvailable("assets/fonts/JetBrainsMono-Regular.ttf");
}

juce::Font FontPack::sans(float size, int styleFlags) const
{
    auto font = juce::Font(juce::FontOptions(size, styleFlags));
    if ((styleFlags & juce::Font::bold) != 0 && sansBold != nullptr)
    {
        auto bold = juce::Font(juce::FontOptions(sansBold));
        bold.setHeight(size);
        bold.setStyleFlags(styleFlags);
        return bold;
    }
    if (sansRegular != nullptr)
    {
        auto regular = juce::Font(juce::FontOptions(sansRegular));
        regular.setHeight(size);
        regular.setStyleFlags(styleFlags);
        return regular;
    }

    return font;
}

juce::Font FontPack::mono(float size, int styleFlags) const
{
    if (monoRegular != nullptr)
    {
        auto mono = juce::Font(juce::FontOptions(monoRegular));
        mono.setHeight(size);
        mono.setStyleFlags(styleFlags);
        return mono;
    }

    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, styleFlags));
}

DesignSystem& DesignSystem::instance()
{
    static DesignSystem s;
    return s;
}

DesignSystem::DesignSystem()
{
    const auto& descriptors = UiThemeRegistry::getBuiltinThemes();
    for (const auto& descriptor : descriptors)
        themesById[descriptor.id] = makeTheme(descriptor);
}

const ThemeTokens& DesignSystem::currentTheme() const
{
    auto it = themesById.find(currentThemeId);
    if (it != themesById.end())
        return it->second;

    return themesById.begin()->second;
}

std::string DesignSystem::resolveThemeId(const juce::String& idOrDisplayName) const
{
    return UiThemeRegistry::resolveThemeId(idOrDisplayName.toStdString());
}

void DesignSystem::setCurrentTheme(const std::string& themeId)
{
    auto resolved = UiThemeRegistry::resolveThemeId(themeId);
    if (themesById.find(resolved) == themesById.end())
        resolved = "studio_one_dark";
    currentThemeId = resolved;
}

juce::StringArray DesignSystem::getThemeDisplayNames() const
{
    juce::StringArray names;
    const auto nativeNames = UiThemeRegistry::getBuiltinDisplayNames();
    for (const auto& name : nativeNames)
        names.add(name);
    return names;
}

juce::String DesignSystem::getDisplayNameForThemeId(const std::string& themeId) const
{
    return UiThemeRegistry::getDisplayName(UiThemeRegistry::resolveThemeId(themeId));
}

const ThemeTokens& DesignSystem::getThemeById(const std::string& themeId) const
{
    auto canonical = UiThemeRegistry::resolveThemeId(themeId);
    auto it = themesById.find(canonical);
    if (it != themesById.end())
        return it->second;
    auto fallback = themesById.find("studio_one_dark");
    if (fallback != themesById.end())
        return fallback->second;
    return themesById.begin()->second;
}

} // namespace Orion::UI
