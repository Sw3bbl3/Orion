#include "orion/UiThemeRegistry.h"

#include <algorithm>
#include <cctype>

namespace Orion {
namespace {

const std::vector<ThemeDescriptor>& builtinThemes()
{
    static const std::vector<ThemeDescriptor> kThemes = {
        {"studio_one_dark", "Deep Space", false, {"Deep Space", "deep space", "studio one dark", "studioone"}},
        {"legacy_dark", "Dark", false, {"Dark", "dark"}},
        {"legacy_light", "Light", true, {"Light", "light"}},
        {"nebula", "Nebula", false, {"Nebula", "nebula"}},
        {"starlight", "Starlight", true, {"Starlight", "starlight"}},
        {"red_giant", "Red Giant", false, {"Red Giant", "red giant"}},
        {"vintage", "Vintage", false, {"Vintage", "vintage"}},
        {"cyberpunk", "Cyberpunk", false, {"Cyberpunk", "cyberpunk"}},
        {"midnight_void", "Midnight Void", false, {"Midnight Void", "midnight void"}}
    };

    return kThemes;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); ++i)
    {
        const auto ac = static_cast<unsigned char>(a[i]);
        const auto bc = static_cast<unsigned char>(b[i]);
        if (std::tolower(ac) != std::tolower(bc))
            return false;
    }

    return true;
}

} // namespace

const std::vector<ThemeDescriptor>& UiThemeRegistry::getBuiltinThemes()
{
    return builtinThemes();
}

bool UiThemeRegistry::isKnownThemeId(const std::string& themeId)
{
    const auto& themes = builtinThemes();
    return std::any_of(themes.begin(), themes.end(), [&themeId](const ThemeDescriptor& t) {
        return t.id == themeId;
    });
}

std::string UiThemeRegistry::resolveThemeId(const std::string& idOrLegacyDisplayName)
{
    const auto& themes = builtinThemes();

    for (const auto& theme : themes)
    {
        if (theme.id == idOrLegacyDisplayName || equalsIgnoreCase(theme.displayName, idOrLegacyDisplayName))
            return theme.id;

        if (std::any_of(theme.legacyNames.begin(), theme.legacyNames.end(), [&idOrLegacyDisplayName](const std::string& legacy) {
                return equalsIgnoreCase(legacy, idOrLegacyDisplayName);
            }))
        {
            return theme.id;
        }
    }

    if (!idOrLegacyDisplayName.empty())
        return idOrLegacyDisplayName;

    // Default flagship theme for v2.
    return "studio_one_dark";
}

std::string UiThemeRegistry::getDisplayName(const std::string& themeId)
{
    const auto& themes = builtinThemes();
    for (const auto& theme : themes)
    {
        if (theme.id == themeId)
            return theme.displayName;
    }

    if (!themeId.empty())
        return themeId;

    return "Deep Space";
}

std::vector<std::string> UiThemeRegistry::getBuiltinDisplayNames()
{
    std::vector<std::string> names;
    const auto& themes = builtinThemes();
    names.reserve(themes.size());

    for (const auto& theme : themes)
        names.push_back(theme.displayName);

    return names;
}

} // namespace Orion
