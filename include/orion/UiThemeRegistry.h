#pragma once

#include <string>
#include <vector>

namespace Orion {

struct ThemeDescriptor {
    std::string id;
    std::string displayName;
    bool isLight = false;
    std::vector<std::string> legacyNames;
};

class UiThemeRegistry {
public:
    static const std::vector<ThemeDescriptor>& getBuiltinThemes();

    static bool isKnownThemeId(const std::string& themeId);
    static std::string resolveThemeId(const std::string& idOrLegacyDisplayName);
    static std::string getDisplayName(const std::string& themeId);
    static std::vector<std::string> getBuiltinDisplayNames();
};

} // namespace Orion

