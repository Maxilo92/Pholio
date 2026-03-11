#pragma once

#include "SettingsWindow.hpp"
#include "ProgressWindow.hpp"
#include "LogWindow.hpp"
#include "DashboardWindow.hpp"
#include "AboutWindow.hpp"
#include "../engine/Worker.hpp"
#include <memory>

namespace ui {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    void update();
    void render();

private:
    void renderMainDockspace();
    
    std::unique_ptr<LogWindow> m_logWindow;
    std::unique_ptr<engine::Worker> m_worker;
    std::unique_ptr<SettingsWindow> m_settingsWindow;
    std::unique_ptr<ProgressWindow> m_progressWindow;
    std::unique_ptr<DashboardWindow> m_dashboardWindow;
    std::unique_ptr<AboutWindow> m_aboutWindow;

    bool m_showDashboard = true;
    bool m_showSettings = false;
    bool m_showProgress = false;
    bool m_showLogs = true;
    bool m_showAbout = false;
};

} // namespace ui
