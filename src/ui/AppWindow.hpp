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
#include "../engine/Worker.hpp"
#include <memory>

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
    void renderMainDockspace();
    void renderStatusBar();
    void setupStyle();
    void checkVersionUpdate();
    void saveWindowState();
    
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

    bool m_showDashboard = true;
    bool m_showSettings = false;
    bool m_showProgress = false;
    bool m_showLogs = true;
    bool m_showPreview = true;
    bool m_showDebug = false;
    bool m_showAbout = false;
    bool m_showChangelog = false;
    bool m_showReport = false;
    bool m_firstRun = false;

    bool m_shouldRestart = false;
    bool m_shouldRebuild = false;
    bool m_shouldClose = false;

public:
    bool shouldRestart() const { return m_shouldRestart; }
    bool shouldRebuild() const { return m_shouldRebuild; }
    bool shouldClose() const { return m_shouldClose; }
    void clearFlags() { m_shouldRestart = m_shouldRebuild = m_shouldClose = false; }
};

} // namespace ui
