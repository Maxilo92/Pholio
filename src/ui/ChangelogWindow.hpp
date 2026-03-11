#pragma once

#include <string>
#include <vector>

namespace ui {

class ChangelogWindow {
public:
    ChangelogWindow() = default;

    void render(bool* p_open);
    
    /**
     * @brief Loads entries from CHANGELOG.md that are newer than sinceVersion.
     * @param sinceVersion The version to start from (exclusive).
     * @return true if new entries were found.
     */
    bool loadNewEntries(const std::string& sinceVersion);

private:
    struct VersionEntry {
        std::string version;
        std::string date;
        std::vector<std::string> changes;
    };

    std::vector<VersionEntry> m_entries;
    std::string m_sinceVersion;
};

} // namespace ui
