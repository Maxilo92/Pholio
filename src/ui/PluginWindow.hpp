#pragma once

#include "../plugins/PluginManager.hpp"
#include <filesystem>
#include <string>

namespace ui {

class PluginWindow {
public:
    enum class Section {
        Search = 0,
        Add = 1,
        Manage = 2
    };

    void setSection(Section section);
    void render(bool* open, plugins::PluginManager* pluginManager);

private:
    void renderSearchSection();
    void renderAddSection();
    void renderManageSection();
    void requestPluginReload();
    bool persistPluginEnabledState(const std::string& pluginName, bool enabled);
    static bool openUrl(const std::string& url);
    static bool openPath(const std::filesystem::path& path, bool ensureExists);

    Section m_section = Section::Manage;
    std::string m_searchQuery = "pholio plugin";
    std::string m_pluginSourcePath;
    std::string m_statusMessage;
    plugins::PluginManager* m_pluginManager = nullptr;
};

} // namespace ui
