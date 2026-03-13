#pragma once

#include "SettingsWindow.hpp"
#include "ProgressWindow.hpp"
#include "LogWindow.hpp"
#include "DashboardWindow.hpp"
#include "PreviewWindow.hpp"
#include "DebugWindow.hpp"
#include "AboutWindow.hpp"
#include "ChangelogWindow.hpp"
#include "ReportWindow.hpp"
#include "PluginWindow.hpp"
#include "plugins/PluginManager.hpp"
#include "../engine/Worker.hpp"
#include <imgui.h>
#include <memory>
#include <string>
#include <filesystem>

#ifdef __APPLE__
#define STR_CTRL "Cmd"
#else
#define STR_CTRL "Ctrl"
#endif

namespace ui {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    void update();
    void render();

private:
    enum class LayoutPreset {
        Default = 0,
        MediaFocus = 1,
        Monitoring = 2
    };

    void renderMainDockspace();
    void renderStatusBar();
    void setupStyle(const std::string& theme);
    void applyLayoutPreset(LayoutPreset preset, ImGuiID dockspaceId, const ImVec2& workSize);
    void checkVersionUpdate();
    void saveWindowState();
    void renderCloseDuringSortingPopup();
    void startNewSort();
    
    std::unique_ptr<LogWindow> m_logWindow;
    std::unique_ptr<engine::Worker> m_worker;
    std::unique_ptr<SettingsWindow> m_settingsWindow;
    std::unique_ptr<ProgressWindow> m_progressWindow;
    std::unique_ptr<DashboardWindow> m_dashboardWindow;
    std::unique_ptr<PreviewWindow> m_previewWindow;
    std::unique_ptr<DebugWindow> m_debugWindow;
    std::unique_ptr<AboutWindow> m_aboutWindow;
    std::unique_ptr<ChangelogWindow> m_changelogWindow;
    std::unique_ptr<ReportWindow> m_reportWindow;
    std::unique_ptr<PluginWindow> m_pluginWindow;
    std::unique_ptr<plugins::PluginManager> m_uiPluginManager;

    bool m_showDashboard = true;
    bool m_showSettings = false;
    bool m_showProgress = false;
    bool m_showLogs = true;
    bool m_showPreview = true;
    bool m_showDebug = false;
    bool m_showAbout = false;
    bool m_showChangelog = false;
    bool m_showReport = false;
    bool m_showPlugins = false;
    bool m_firstRun = false;

    bool m_shouldRestart = false;
    bool m_shouldRebuild = false;
    bool m_shouldClose = false;
    bool m_showCloseDuringSortingPopup = false;
    std::string m_updateActionMessage;
    bool m_uiPluginsEnabled = false;
    bool m_uiPluginWindowsAllowed = true;
    std::filesystem::path m_uiPluginsDirectory;
    int m_uiPluginReloadToken = 0;
    std::string m_uiTheme = "dark";
    int m_pendingLayoutPreset = -1;

public:
    bool shouldRestart() const { return m_shouldRestart; }
    bool shouldRebuild() const { return m_shouldRebuild; }
    bool shouldClose() const { return m_shouldClose; }
    bool isSortingActive() const { return m_worker->isRunning(); }
    void requestClose() { m_shouldClose = true; }
    void clearFlags() { m_shouldRestart = m_shouldRebuild = m_shouldClose = false; }
};

} // namespace ui
