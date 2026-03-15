#include "PluginWindow.hpp"
#include <imgui.h>
#include "../core/ConfigManager.hpp"
#include "I18n.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace ui {

namespace {
bool hasPluginExtension(const std::filesystem::path& path) {
#ifdef _WIN32
    return path.extension() == ".dll";
#elif __APPLE__
    return path.extension() == ".dylib" || path.extension() == ".so";
#else
    return path.extension() == ".so";
#endif
}

bool runOpenCommand(const std::string& target) {
#ifdef _WIN32
    return std::system(("start \"\" \"" + target + "\"").c_str()) == 0;
#elif __APPLE__
    return std::system(("open \"" + target + "\"").c_str()) == 0;
#else
    return std::system(("xdg-open \"" + target + "\"").c_str()) == 0;
#endif
}
} // namespace

void PluginWindow::setSection(Section section) {
    m_section = section;
}

void PluginWindow::render(bool* open, plugins::PluginManager* pluginManager) {
    auto tr = [](const char* key, const char* fallback) { return i18n::tr(key, fallback); };
    m_pluginManager = pluginManager;
    if (!ImGui::Begin(tr("plugin.window.title", "Plugins"), open)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button(tr("plugin.tab.search.icon", "[?] Search"), ImVec2(100, 0))) {
        m_section = Section::Search;
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("plugin.tab.add.icon", "[+] Add"), ImVec2(100, 0))) {
        m_section = Section::Add;
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("plugin.tab.manage.icon", "[*] Manage"), ImVec2(100, 0))) {
        m_section = Section::Manage;
    }
    ImGui::Separator();

    if (m_section == Section::Search) {
        renderSearchSection();
    } else if (m_section == Section::Add) {
        renderAddSection();
    } else {
        renderManageSection();
    }

    if (!m_statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextWrapped("%s", m_statusMessage.c_str());
    }

    ImGui::End();
}

void PluginWindow::renderSearchSection() {
    auto tr = [](const char* key, const char* fallback) { return i18n::tr(key, fallback); };
    char searchBuf[256];
    std::strncpy(searchBuf, m_searchQuery.c_str(), sizeof(searchBuf));
    if (ImGui::InputText(tr("plugin.search.input", "GitHub Search"), searchBuf, sizeof(searchBuf))) {
        m_searchQuery = searchBuf;
    }
    ImGui::SetItemTooltip("%s", tr("plugin.search.tooltip", "Search query for plugin discovery on GitHub."));

    if (ImGui::Button(tr("plugin.search.button", "Search on GitHub"))) {
        std::string query = m_searchQuery;
        std::replace(query.begin(), query.end(), ' ', '+');
        if (!openUrl("https://github.com/search?q=" + query + "&type=repositories")) {
            m_statusMessage = tr("plugin.search.fail", "Failed to open browser for GitHub search.");
        } else {
            m_statusMessage = tr("plugin.search.success", "Opened GitHub search in browser.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("plugin.search.hub", "Browse Plugin Hub"))) {
        if (!openUrl("https://github.com/topics/pholio-plugin")) {
            m_statusMessage = tr("plugin.hub.fail", "Failed to open plugin hub.");
        } else {
            m_statusMessage = tr("plugin.hub.success", "Opened plugin hub in browser.");
        }
    }
}

void PluginWindow::renderAddSection() {
    auto tr = [](const char* key, const char* fallback) { return i18n::tr(key, fallback); };
    auto& config = core::ConfigManager::getInstance();
    auto settings = config.getSettings();

    ImGui::TextWrapped("%s", tr("plugin.add.description", "Add a compiled plugin library into your configured plugin directory."));
    ImGui::Spacing();

    char srcBuf[1024];
    std::strncpy(srcBuf, m_pluginSourcePath.c_str(), sizeof(srcBuf));
    if (ImGui::InputText(tr("plugin.add.path", "Plugin file path"), srcBuf, sizeof(srcBuf))) {
        m_pluginSourcePath = srcBuf;
    }
    ImGui::SetItemTooltip("%s", tr("plugin.add.path.tooltip", "Path to a .dylib/.so/.dll file."));

    if (ImGui::Button(tr("plugin.add.button", "Add Plugin"))) {
        try {
            const std::filesystem::path sourcePath = m_pluginSourcePath;
            if (sourcePath.empty() || !std::filesystem::exists(sourcePath) || !std::filesystem::is_regular_file(sourcePath)) {
                m_statusMessage = tr("plugin.add.invalid_path", "Invalid plugin file path.");
            } else if (!hasPluginExtension(sourcePath)) {
                m_statusMessage = tr("plugin.add.unsupported_ext", "Unsupported plugin extension for this platform.");
            } else {
                std::filesystem::create_directories(settings.pluginsDirectory);
                const std::filesystem::path targetPath = settings.pluginsDirectory / sourcePath.filename();
                std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::overwrite_existing);
                m_statusMessage = std::string(tr("plugin.add.added", "Plugin added: ")) + targetPath.string();
                requestPluginReload();
            }
        } catch (const std::exception& e) {
            m_statusMessage = std::string(tr("plugin.add.fail", "Failed to add plugin: ")) + e.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("plugin.add.open_folder", "Open Plugin Folder"))) {
        if (!openPath(settings.pluginsDirectory, true)) {
            m_statusMessage = tr("plugin.folder.open_fail", "Failed to open plugin folder.");
        } else {
            m_statusMessage = tr("plugin.folder.open_ok", "Opened plugin folder.");
        }
    }
}

void PluginWindow::renderManageSection() {
    auto tr = [](const char* key, const char* fallback) { return i18n::tr(key, fallback); };
    auto& config = core::ConfigManager::getInstance();
    auto settings = config.getSettings();
    auto renderHint = [&](const char* key, const char* fallback) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::SetTooltip("%s", tr(key, fallback));
        }
    };

    ImGui::TextWrapped("%s", tr("plugin.manage.guide",
                                "Guided workflow: 1) Prepare plugin folder 2) Add library files 3) Reload plugins 4) Enable and open plugin windows."));
    ImGui::Spacing();

    ImGui::Text("%s", tr("plugin.manage.configured_dir", "Configured plugin directory:"));
    renderHint("plugin.manage.configured_dir.hint",
               "This is where .dylib/.so/.dll plugin files are loaded from.");
    ImGui::TextWrapped("%s", settings.pluginsDirectory.string().c_str());
    ImGui::Spacing();

    if (ImGui::Button(tr("plugin.manage.auto_setup", "Auto Setup (Recommended)"))) {
        try {
            std::filesystem::create_directories(settings.pluginsDirectory);
            const bool opened = openPath(settings.pluginsDirectory, false);
            requestPluginReload();
            m_statusMessage = opened
                ? tr("plugin.manage.auto_setup.ok", "Plugin folder prepared, opened, and plugin reload requested.")
                : tr("plugin.manage.auto_setup.partial", "Plugin folder prepared and reload requested, but folder could not be opened.");
        } catch (const std::exception& e) {
            m_statusMessage = std::string(tr("plugin.manage.auto_setup.fail", "Auto setup failed: ")) + e.what();
        }
    }
    renderHint("plugin.manage.auto_setup.hint",
               "Automates folder creation, opening the folder, and triggering plugin reload.");

    ImGui::Spacing();

    if (ImGui::Button(tr("plugin.manage.open_folder", "Open Folder"))) {
        if (!openPath(settings.pluginsDirectory, true)) {
            m_statusMessage = tr("plugin.folder.open_fail", "Failed to open plugin folder.");
        } else {
            m_statusMessage = tr("plugin.folder.open_ok", "Opened plugin folder.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("plugin.manage.create_folder", "Create Folder"))) {
        try {
            std::filesystem::create_directories(settings.pluginsDirectory);
            m_statusMessage = tr("plugin.manage.folder_ready", "Plugin directory is ready.");
            requestPluginReload();
        } catch (const std::exception& e) {
            m_statusMessage = std::string(tr("plugin.manage.create_fail", "Failed to create plugin directory: ")) + e.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("plugin.manage.reload", "Reload Plugins"))) {
        requestPluginReload();
        m_statusMessage = tr("plugin.manage.reload_requested", "Plugin reload requested.");
    }

    int detectedCount = 0;
    if (std::filesystem::exists(settings.pluginsDirectory) && std::filesystem::is_directory(settings.pluginsDirectory)) {
        for (const auto& entry : std::filesystem::directory_iterator(settings.pluginsDirectory)) {
            if (entry.is_regular_file() && hasPluginExtension(entry.path())) {
                detectedCount++;
            }
        }
    }

    const auto descriptors = m_pluginManager ? m_pluginManager->getPluginDescriptors() : std::vector<plugins::PluginManager::PluginDescriptor>{};
    int enabledCount = 0;
    int windowCapableCount = 0;
    for (const auto& descriptor : descriptors) {
        if (descriptor.enabled) {
            enabledCount++;
        }
        if (descriptor.hasWindow) {
            windowCapableCount++;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", tr("plugin.manage.summary", "Manager summary:"));
    ImGui::BulletText("%s %d", tr("plugin.manage.summary.detected", "Detected libraries:"), detectedCount);
    ImGui::BulletText("%s %d", tr("plugin.manage.summary.loaded", "Loaded plugins:"), static_cast<int>(descriptors.size()));
    ImGui::BulletText("%s %d", tr("plugin.manage.summary.enabled", "Enabled plugins:"), enabledCount);
    ImGui::BulletText("%s %d", tr("plugin.manage.summary.windows", "Plugins with windows:"), windowCapableCount);

    ImGui::Separator();
    ImGui::Text("%s", tr("plugin.manage.detected_libs", "Detected plugin libraries:"));

    int count = 0;
    if (std::filesystem::exists(settings.pluginsDirectory) && std::filesystem::is_directory(settings.pluginsDirectory)) {
        for (const auto& entry : std::filesystem::directory_iterator(settings.pluginsDirectory)) {
            if (!entry.is_regular_file() || !hasPluginExtension(entry.path())) {
                continue;
            }
            ImGui::BulletText("%s", entry.path().filename().string().c_str());
            count++;
        }
    }
    if (count == 0) {
        ImGui::TextDisabled("%s", tr("plugin.manage.no_plugins", "No plugins found."));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", tr("plugin.manage.loaded_plugins", "Loaded plugins (toggle):"));
    renderHint("plugin.manage.loaded_plugins.hint",
               "Use checkboxes to enable or disable plugins quickly. Disabled plugins are persisted in settings.");
    if (!m_pluginManager) {
        ImGui::TextDisabled("%s", tr("plugin.manage.manager_missing", "Plugin manager not available."));
        return;
    }

    if (descriptors.empty()) {
        ImGui::TextDisabled("%s", tr("plugin.manage.no_loaded_plugins", "No loaded plugins."));
    } else {
        if (ImGui::Button(tr("plugin.manage.enable_all", "Enable All"))) {
            const int changed = setAllLoadedPluginsEnabled(true);
            m_statusMessage = std::string(tr("plugin.manage.enable_all.ok", "Enabled plugins: ")) + std::to_string(changed);
        }
        ImGui::SameLine();
        if (ImGui::Button(tr("plugin.manage.disable_all", "Disable All"))) {
            const int changed = setAllLoadedPluginsEnabled(false);
            m_statusMessage = std::string(tr("plugin.manage.disable_all.ok", "Disabled plugins: ")) + std::to_string(changed);
        }
        ImGui::Spacing();

        char filterBuf[256];
        std::strncpy(filterBuf, m_manageFilter.c_str(), sizeof(filterBuf));
        if (ImGui::InputText(tr("plugin.manage.filter", "Filter plugins"), filterBuf, sizeof(filterBuf))) {
            m_manageFilter = filterBuf;
        }

        for (const auto& descriptor : descriptors) {
            if (!m_manageFilter.empty() && descriptor.name.find(m_manageFilter) == std::string::npos) {
                continue;
            }

            bool enabled = descriptor.enabled;
            const std::string checkboxId = "##plugin_enabled_" + descriptor.name;
            if (ImGui::Checkbox(checkboxId.c_str(), &enabled)) {
                if (m_pluginManager->setPluginEnabled(descriptor.name, enabled) && persistPluginEnabledState(descriptor.name, enabled)) {
                    if (enabled) {
                        m_statusMessage = std::string(tr("plugin.manage.enabled", "Enabled plugin: ")) + descriptor.name;
                    } else {
                        m_statusMessage = std::string(tr("plugin.manage.disabled", "Disabled plugin: ")) + descriptor.name;
                    }
                } else {
                    m_statusMessage = std::string(tr("plugin.manage.toggle_fail", "Failed to change plugin state: ")) + descriptor.name;
                }
            }
            ImGui::SameLine();
            ImGui::Text("%s", descriptor.name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", descriptor.enabled ? tr("plugin.state.enabled", "(enabled)") : tr("plugin.state.disabled", "(disabled)"));
            ImGui::BulletText("%s %s | %s %s",
                              tr("plugin.manage.meta.version", "Version:"),
                              descriptor.version.c_str(),
                              tr("plugin.manage.meta.author", "Author:"),
                              descriptor.author.c_str());
            ImGui::BulletText("%s %s",
                              tr("plugin.manage.meta.window", "Window support:"),
                              descriptor.hasWindow ? tr("plugin.manage.meta.window.yes", "yes") : tr("plugin.manage.meta.window.no", "no"));
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", tr("plugin.manage.windows", "Plugin windows:"));

    const auto windowStates = m_pluginManager->getPluginWindowStates();
    if (windowStates.empty()) {
        ImGui::TextDisabled("%s", tr("plugin.manage.no_windows", "No plugin windows exposed."));
        return;
    }

    for (const auto& state : windowStates) {
        ImGui::Text("%s", state.name.c_str());
        ImGui::SameLine();
        if (!state.enabled) {
            ImGui::TextDisabled("%s", tr("plugin.state.disabled", "(disabled)"));
            continue;
        }
        if (state.open) {
            ImGui::TextDisabled("%s", tr("plugin.state.open", "(open)"));
        } else {
            const std::string btn = std::string(tr("plugin.window.reopen", "Reopen")) + "##" + state.name;
            if (ImGui::Button(btn.c_str())) {
                if (m_pluginManager->reopenPluginWindow(state.name)) {
                    m_statusMessage = std::string(tr("plugin.window.reopened", "Reopened plugin window: ")) + state.name;
                } else {
                    m_statusMessage = std::string(tr("plugin.window.reopen_fail", "Could not reopen plugin window: ")) + state.name;
                }
            }
        }
    }
}

bool PluginWindow::openUrl(const std::string& url) {
    return runOpenCommand(url);
}

bool PluginWindow::openPath(const std::filesystem::path& path, bool ensureExists) {
    try {
        if (ensureExists) {
            std::filesystem::create_directories(path);
        }
        const auto absolutePath = std::filesystem::absolute(path);
        return runOpenCommand(absolutePath.string());
    } catch (...) {
        return false;
    }
}

void PluginWindow::requestPluginReload() {
    auto& config = core::ConfigManager::getInstance();
    auto settings = config.getSettings();
    settings.pluginReloadToken += 1;
    config.setSettings(settings);
    config.save();
}

bool PluginWindow::persistPluginEnabledState(const std::string& pluginName, bool enabled) {
    auto& config = core::ConfigManager::getInstance();
    auto settings = config.getSettings();
    auto& disabled = settings.disabledPlugins;
    disabled.erase(std::remove(disabled.begin(), disabled.end(), pluginName), disabled.end());
    if (!enabled) {
        disabled.push_back(pluginName);
    }
    std::sort(disabled.begin(), disabled.end());
    disabled.erase(std::unique(disabled.begin(), disabled.end()), disabled.end());
    config.setSettings(settings);
    config.save();
    return true;
}

int PluginWindow::setAllLoadedPluginsEnabled(bool enabled) {
    if (!m_pluginManager) {
        return 0;
    }

    int changed = 0;
    for (const auto& descriptor : m_pluginManager->getPluginDescriptors()) {
        if (descriptor.enabled == enabled) {
            continue;
        }
        if (m_pluginManager->setPluginEnabled(descriptor.name, enabled) &&
            persistPluginEnabledState(descriptor.name, enabled)) {
            changed++;
        }
    }
    return changed;
}

} // namespace ui
