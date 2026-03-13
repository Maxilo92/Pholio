#include "AppWindow.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include "core/ConfigManager.hpp"
#include "core/Config.hpp"
#include "core/UpdateManager.hpp"

namespace ui {

AppWindow::AppWindow() 
    : m_logWindow(std::make_unique<LogWindow>()),
      m_worker(std::make_unique<engine::Worker>(*m_logWindow)),
      m_settingsWindow(std::make_unique<SettingsWindow>()),
      m_progressWindow(std::make_unique<ProgressWindow>(*m_worker)),
      m_dashboardWindow(std::make_unique<DashboardWindow>(*m_worker)),
      m_previewWindow(std::make_unique<PreviewWindow>(*m_worker)),
      m_debugWindow(std::make_unique<DebugWindow>()),
      m_aboutWindow(std::make_unique<AboutWindow>()),
      m_changelogWindow(std::make_unique<ChangelogWindow>()),
      m_reportWindow(std::make_unique<ReportWindow>()),
      m_shouldRestart(false),
      m_shouldRebuild(false),
      m_shouldClose(false) {
    
    // Check if it's the first run
    try {
        if (!std::filesystem::exists(core::ConfigManager::getInstance().getConfigPath())) {
            m_firstRun = true;
        }
    } catch (...) {
        m_firstRun = true;
    }

    // Load visibility settings
    auto settings = core::ConfigManager::getInstance().getSettings();
    m_showDashboard = settings.showDashboard;
    m_showSettings = settings.showSettings;
    m_showProgress = settings.showProgress;
    m_showLogs = settings.showLogs;
    m_showPreview = settings.showPreview;
    m_showReport = settings.showReport;

    m_logWindow->setupFileLogging(core::ConfigManager::getInstance().getLogDirectory());
    setupStyle();
    checkVersionUpdate();

    // Start checking for updates in background
    core::UpdateManager::getInstance().checkForUpdates();
}

AppWindow::~AppWindow() = default;

void AppWindow::checkVersionUpdate() {
    auto& config = core::ConfigManager::getInstance();
    auto settings = config.getSettings();
    std::string currentVersion(core::PROJECT_VERSION);

    if (settings.lastVersion.empty()) {
        settings.lastVersion = currentVersion;
        config.setSettings(settings);
        config.save();
        return;
    }

    if (settings.lastVersion != currentVersion) {
        if (m_changelogWindow->loadNewEntries(settings.lastVersion)) {
            m_showChangelog = true;
        }
        settings.lastVersion = currentVersion;
        config.setSettings(settings);
        config.save();
    }
}

void AppWindow::update() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

#ifdef __APPLE__
    bool cmd_down = io.KeySuper;
#else
    bool cmd_down = io.KeyCtrl;
#endif

    static const auto shortcutsDelay = []() {
        const char* relaunchedEnv = std::getenv("PHOLIO_RELAUNCHED");
        const bool isRelaunched = relaunchedEnv != nullptr && std::string(relaunchedEnv) == "1";
        return std::chrono::seconds(isRelaunched ? 15 : 3);
    }();
    static const auto shortcutsEnabledAt = std::chrono::steady_clock::now() + shortcutsDelay;
    const bool shortcutsEnabled = std::chrono::steady_clock::now() >= shortcutsEnabledAt;

    // Defer global shortcuts briefly after startup to avoid phantom modifier/key events.
    if (cmd_down && shortcutsEnabled) {
        if (ImGui::IsKeyPressed(ImGuiKey_D, false)) { m_showDashboard = !m_showDashboard; saveWindowState(); }
        if (ImGui::IsKeyPressed(ImGuiKey_S, false)) { m_showSettings = !m_showSettings; saveWindowState(); }
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            m_shouldRestart = true;
            if (io.KeyShift) m_shouldRebuild = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
            m_shouldClose = true;
        }
    }
}

void AppWindow::render() {
    renderMainDockspace();

    const auto updateInfo = core::UpdateManager::getInstance().getUpdateInfo();
    if (updateInfo.hasUpdate) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->Pos.x + 20.0f, ImGui::GetMainViewport()->Pos.y + 40.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350.0f, 0.0f));
        if (ImGui::Begin("Update Available", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "A new version is available: v%s", updateInfo.latestVersion.c_str());
            ImGui::Spacing();
            const bool sortingActive = m_worker->isRunning();

            if (updateInfo.canInstallDirectly) {
                if (sortingActive) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "Update is blocked while sorting is running.");
                }

                ImGui::BeginDisabled(sortingActive);
                if (ImGui::Button("Update and Restart")) {
                    if (core::UpdateManager::getInstance().installQueuedUpdateNow()) {
                        m_updateActionMessage = "Installing update and closing app...";
                        m_shouldClose = true;
                    } else {
                        m_updateActionMessage = core::UpdateManager::getInstance().getLastInstallError();
                        if (m_updateActionMessage.empty()) {
                            m_updateActionMessage = "Update installation could not be started.";
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Update at Restart")) {
                    if (core::UpdateManager::getInstance().queueUpdateForNextRestart()) {
                        m_updateActionMessage = "Update queued for next restart.";
                    } else {
                        m_updateActionMessage = "No installable update package found.";
                    }
                }
                ImGui::EndDisabled();
                ImGui::Spacing();
            } else {
                ImGui::TextDisabled("Direct install package not found in this release.");
                ImGui::Spacing();
            }

            if (!m_updateActionMessage.empty()) {
                ImGui::TextWrapped("%s", m_updateActionMessage.c_str());
                ImGui::Spacing();
            }

            if (ImGui::Button("View on GitHub")) {
#ifdef _WIN32
                std::system(("start " + updateInfo.releaseUrl).c_str());
#else
                std::system(("open " + updateInfo.releaseUrl).c_str());
#endif
            }
            ImGui::SameLine();
            if (ImGui::Button("Dismiss")) {
                core::UpdateManager::getInstance().reset();
            }
        }
        ImGui::End();
    }

    if (m_showDashboard) m_dashboardWindow->render();
    if (m_showSettings) m_settingsWindow->render();
    if (m_showProgress) m_progressWindow->render();
    if (m_showLogs) m_logWindow->render();
    if (m_showPreview) m_previewWindow->render(&m_showPreview);
    if (m_showDebug) m_debugWindow->render(&m_showDebug);
    if (m_showAbout) m_aboutWindow->render(&m_showAbout);
    if (m_showChangelog) m_changelogWindow->render(&m_showChangelog);
    if (m_showReport) m_reportWindow->render(&m_showReport);

    static bool lastDashboard = m_showDashboard;
    static bool lastSettings = m_showSettings;
    static bool lastProgress = m_showProgress;
    static bool lastLogs = m_showLogs;
    static bool lastPreview = m_showPreview;
    static bool lastReport = m_showReport;

    if (lastDashboard != m_showDashboard || lastSettings != m_showSettings || 
        lastProgress != m_showProgress || lastLogs != m_showLogs || 
        lastPreview != m_showPreview || lastReport != m_showReport) {
        saveWindowState();
        lastDashboard = m_showDashboard;
        lastSettings = m_showSettings;
        lastProgress = m_showProgress;
        lastLogs = m_showLogs;
        lastPreview = m_showPreview;
        lastReport = m_showReport;
    }

    renderStatusBar();
}

void AppWindow::saveWindowState() {
    auto& config = core::ConfigManager::getInstance();
    auto settings = config.getSettings();
    settings.showDashboard = m_showDashboard;
    settings.showSettings = m_showSettings;
    settings.showProgress = m_showProgress;
    settings.showLogs = m_showLogs;
    settings.showPreview = m_showPreview;
    settings.showReport = m_showReport;
    config.setSettings(settings);
    config.save();
}

void AppWindow::renderMainDockspace() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("MainDockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }

    if (m_firstRun) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);
        
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, NULL, &dock_main_id);
        ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, NULL, &dock_main_id);
        
        ImGui::DockBuilderDockWindow("Dashboard", dock_main_id);
        ImGui::DockBuilderDockWindow("Image Preview", dock_main_id);
        ImGui::DockBuilderDockWindow("Logs", dock_id_bottom);
        ImGui::DockBuilderDockWindow("Settings", dock_id_right);
        ImGui::DockBuilderDockWindow("Progress & Performance", dock_id_right);
        ImGui::DockBuilderDockWindow("Internal Debug", dock_id_bottom);
        
        ImGui::DockBuilderFinish(dockspace_id);
        m_firstRun = false;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Restart", STR_CTRL "+R")) m_shouldRestart = true;
            if (ImGui::MenuItem("Exit", STR_CTRL "+Q")) m_shouldClose = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Dashboard", nullptr, &m_showDashboard);
            ImGui::MenuItem("Image Preview", nullptr, &m_showPreview);
            ImGui::MenuItem("Settings", nullptr, &m_showSettings);
            ImGui::MenuItem("Progress & Performance", nullptr, &m_showProgress);
            ImGui::MenuItem("Logs", nullptr, &m_showLogs);
            ImGui::Separator();
            ImGui::MenuItem("Internal Debug", nullptr, &m_showDebug);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) m_firstRun = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Check for Updates")) {
                core::UpdateManager::getInstance().checkForUpdates();
            }
            if (ImGui::MenuItem("Report Issue")) m_showReport = true;
            if (ImGui::MenuItem("What's New")) { 
                m_changelogWindow->loadNewEntries("0.0.0"); 
                m_showChangelog = true; 
            }
            if (ImGui::MenuItem("About")) m_showAbout = true;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void AppWindow::renderStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeightWithSpacing()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeightWithSpacing()));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("StatusBar", nullptr, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            ImGui::Text("%s v%s", core::PROJECT_NAME.data(), core::PROJECT_VERSION.data());
            ImGui::Separator();
            if (m_worker->isRunning()) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: Running");
            } else {
                ImGui::Text("Status: Idle");
            }
            
            if (core::UpdateManager::getInstance().isChecking()) {
                ImGui::Separator();
                ImGui::TextDisabled("Checking for updates...");
            } else if (core::UpdateManager::getInstance().getUpdateInfo().hasUpdate) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Update Available!");
            }

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

void AppWindow::setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;

    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
}

} // namespace ui
