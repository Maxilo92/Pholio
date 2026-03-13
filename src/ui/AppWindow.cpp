#include "AppWindow.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include "core/ConfigManager.hpp"
#include "core/Config.hpp"
#include "core/UpdateManager.hpp"
#include "I18n.hpp"

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
      m_pluginWindow(std::make_unique<PluginWindow>()),
      m_uiPluginManager(std::make_unique<plugins::PluginManager>(*m_logWindow)),
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
    m_showPlugins = settings.showPlugins;
    ui::i18n::setLanguage(ui::i18n::languageFromCode(settings.uiLanguage));
    m_uiTheme = settings.uiTheme;
    m_uiPluginsEnabled = settings.enablePlugins;
    m_uiPluginWindowsAllowed = settings.allowPluginWindows;
    m_uiPluginsDirectory = settings.pluginsDirectory;
    m_uiPluginReloadToken = settings.pluginReloadToken;
    if (m_uiPluginsEnabled) {
        m_uiPluginManager->setDisabledPlugins(settings.disabledPlugins);
        m_uiPluginManager->loadFromDirectory(m_uiPluginsDirectory);
    }

    m_logWindow->setupFileLogging(core::ConfigManager::getInstance().getLogDirectory());
    setupStyle(m_uiTheme);
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
    const auto currentSettings = core::ConfigManager::getInstance().getSettings();
    i18n::setLanguage(i18n::languageFromCode(currentSettings.uiLanguage));
    if (currentSettings.uiTheme != m_uiTheme) {
        m_uiTheme = currentSettings.uiTheme;
        setupStyle(m_uiTheme);
    }
    if (currentSettings.enablePlugins != m_uiPluginsEnabled ||
        currentSettings.pluginsDirectory != m_uiPluginsDirectory ||
        currentSettings.pluginReloadToken != m_uiPluginReloadToken) {
        m_uiPluginsEnabled = currentSettings.enablePlugins;
        m_uiPluginsDirectory = currentSettings.pluginsDirectory;
        m_uiPluginReloadToken = currentSettings.pluginReloadToken;
        if (m_uiPluginsEnabled) {
            m_uiPluginManager->setDisabledPlugins(currentSettings.disabledPlugins);
            m_uiPluginManager->loadFromDirectory(m_uiPluginsDirectory);
        }
    }
    m_uiPluginManager->setDisabledPlugins(currentSettings.disabledPlugins);
    m_uiPluginWindowsAllowed = currentSettings.allowPluginWindows;

    renderMainDockspace();

    const auto updateInfo = core::UpdateManager::getInstance().getUpdateInfo();
    if (updateInfo.hasUpdate) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->Pos.x + 20.0f, ImGui::GetMainViewport()->Pos.y + 40.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350.0f, 0.0f));
        if (ImGui::Begin(i18n::tr("update.available", "Update Available"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), i18n::tr("update.new_version", "A new version is available: v%s"), updateInfo.latestVersion.c_str());
            ImGui::Spacing();
            const bool sortingActive = m_worker->isRunning();

            if (updateInfo.canInstallDirectly) {
                if (sortingActive) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "%s", i18n::tr("update.blocked", "Update is blocked while sorting is running."));
                }

                ImGui::BeginDisabled(sortingActive);
                if (ImGui::Button(i18n::tr("update.install_restart", "Update and Restart"))) {
                    if (core::UpdateManager::getInstance().installQueuedUpdateNow()) {
                        m_updateActionMessage = i18n::tr("update.installing", "Installing update and closing app...");
                        m_shouldClose = true;
                    } else {
                        m_updateActionMessage = core::UpdateManager::getInstance().getLastInstallError();
                        if (m_updateActionMessage.empty()) {
                            m_updateActionMessage = i18n::tr("update.install_failed", "Update installation could not be started.");
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(i18n::tr("update.install_later", "Update at Restart"))) {
                    if (core::UpdateManager::getInstance().queueUpdateForNextRestart()) {
                        m_updateActionMessage = i18n::tr("update.queued", "Update queued for next restart.");
                    } else {
                        m_updateActionMessage = i18n::tr("update.no_package", "No installable update package found.");
                    }
                }
                ImGui::EndDisabled();
                ImGui::Spacing();
            } else {
                ImGui::TextDisabled("%s", i18n::tr("update.no_direct_package", "Direct install package not found in this release."));
                ImGui::Spacing();
            }

            if (!m_updateActionMessage.empty()) {
                ImGui::TextWrapped("%s", m_updateActionMessage.c_str());
                ImGui::Spacing();
            }

            if (ImGui::Button(i18n::tr("update.view_github", "View on GitHub"))) {
#ifdef _WIN32
                std::system(("start " + updateInfo.releaseUrl).c_str());
#else
                std::system(("open " + updateInfo.releaseUrl).c_str());
#endif
            }
            ImGui::SameLine();
            if (ImGui::Button(i18n::tr("update.dismiss", "Dismiss"))) {
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
    if (m_showPlugins) m_pluginWindow->render(&m_showPlugins, m_uiPluginManager.get());
    if (m_uiPluginsEnabled && m_uiPluginWindowsAllowed) m_uiPluginManager->renderWindows();

    static bool lastDashboard = m_showDashboard;
    static bool lastSettings = m_showSettings;
    static bool lastProgress = m_showProgress;
    static bool lastLogs = m_showLogs;
    static bool lastPreview = m_showPreview;
    static bool lastReport = m_showReport;
    static bool lastPlugins = m_showPlugins;

    if (lastDashboard != m_showDashboard || lastSettings != m_showSettings || 
        lastProgress != m_showProgress || lastLogs != m_showLogs || 
        lastPreview != m_showPreview || lastReport != m_showReport || lastPlugins != m_showPlugins) {
        saveWindowState();
        lastDashboard = m_showDashboard;
        lastSettings = m_showSettings;
        lastProgress = m_showProgress;
        lastLogs = m_showLogs;
        lastPreview = m_showPreview;
        lastReport = m_showReport;
        lastPlugins = m_showPlugins;
    }

    if (m_shouldClose && m_worker->isRunning()) {
        m_showCloseDuringSortingPopup = true;
        m_shouldClose = false;
        m_worker->requestPause();
    }

    renderCloseDuringSortingPopup();
    renderStatusBar();
}

void AppWindow::renderCloseDuringSortingPopup() {
    if (m_showCloseDuringSortingPopup) {
        ImGui::OpenPopup(i18n::tr("close_sorting.title", "Close While Sorting"));
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (m_showCloseDuringSortingPopup && !m_worker->isRunning()) {
        m_showCloseDuringSortingPopup = false;
        ImGui::CloseCurrentPopup();
        return;
    }

    if (ImGui::BeginPopupModal(i18n::tr("close_sorting.title", "Close While Sorting"), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", i18n::tr("close_sorting.line1", "Sorting is paused. To protect your files, choose whether to continue or stop and close."));
        ImGui::Spacing();
        ImGui::TextWrapped("%s", i18n::tr("close_sorting.line2", "Do you want to continue sorting or stop now and exit?"));
        ImGui::Spacing();
        if (ImGui::Button(i18n::tr("close_sorting.continue", "Continue Sorting"), ImVec2(160, 0))) {
            m_worker->resumeFromPause();
            m_showCloseDuringSortingPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(i18n::tr("close_sorting.stop_exit", "Stop and Exit"), ImVec2(160, 0))) {
            m_worker->stop();
            m_shouldClose = true;
            m_showCloseDuringSortingPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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
    settings.showPlugins = m_showPlugins;
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

    if (m_pendingLayoutPreset >= 0) {
        applyLayoutPreset(static_cast<LayoutPreset>(m_pendingLayoutPreset), dockspace_id, viewport->WorkSize);
        m_pendingLayoutPreset = -1;
    }

    if (m_firstRun) {
        applyLayoutPreset(LayoutPreset::Default, dockspace_id, viewport->WorkSize);
        m_firstRun = false;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu(i18n::tr("menu.file", "File"))) {
            if (ImGui::MenuItem(i18n::tr("menu.restart", "Restart"), STR_CTRL "+R")) m_shouldRestart = true;
            if (ImGui::MenuItem(i18n::tr("menu.exit", "Exit"), STR_CTRL "+Q")) m_shouldClose = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(i18n::tr("menu.view", "View"))) {
            ImGui::MenuItem(i18n::tr("menu.dashboard.icon", "[D] Dashboard"), nullptr, &m_showDashboard);
            ImGui::MenuItem(i18n::tr("menu.preview.icon", "[P] Image Preview"), nullptr, &m_showPreview);
            ImGui::MenuItem(i18n::tr("menu.settings.icon", "[S] Settings"), nullptr, &m_showSettings);
            ImGui::MenuItem(i18n::tr("menu.progress.icon", "[M] Progress & Performance"), nullptr, &m_showProgress);
            ImGui::MenuItem(i18n::tr("menu.logs.icon", "[L] Logs"), nullptr, &m_showLogs);
            ImGui::MenuItem(i18n::tr("menu.plugins.icon", "[G] Plugins"), nullptr, &m_showPlugins);
            ImGui::Separator();
            ImGui::MenuItem(i18n::tr("menu.internal_debug", "Internal Debug"), nullptr, &m_showDebug);
            ImGui::Separator();
            if (ImGui::BeginMenu(i18n::tr("menu.layout_presets", "Layout Presets"))) {
                if (ImGui::MenuItem(i18n::tr("menu.layout.default", "Default"))) m_pendingLayoutPreset = static_cast<int>(LayoutPreset::Default);
                if (ImGui::MenuItem(i18n::tr("menu.layout.media", "Media Focus"))) m_pendingLayoutPreset = static_cast<int>(LayoutPreset::MediaFocus);
                if (ImGui::MenuItem(i18n::tr("menu.layout.monitor", "Monitoring Focus"))) m_pendingLayoutPreset = static_cast<int>(LayoutPreset::Monitoring);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(i18n::tr("menu.reset_layout", "Reset Layout"))) m_firstRun = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(i18n::tr("menu.plugins", "Plugins"))) {
            if (ImGui::MenuItem(i18n::tr("menu.plugin.search.icon", "[?] Search"))) {
                m_showPlugins = true;
                m_pluginWindow->setSection(PluginWindow::Section::Search);
            }
            if (ImGui::MenuItem(i18n::tr("menu.plugin.add.icon", "[+] Add"))) {
                m_showPlugins = true;
                m_pluginWindow->setSection(PluginWindow::Section::Add);
            }
            if (ImGui::MenuItem(i18n::tr("menu.plugin.manage.icon", "[*] Manage"))) {
                m_showPlugins = true;
                m_pluginWindow->setSection(PluginWindow::Section::Manage);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(i18n::tr("menu.plugin.browse_hub", "Browse Plugin Hub"))) {
#ifdef _WIN32
                std::system("start \"\" \"https://github.com/topics/pholio-plugin\"");
#elif __APPLE__
                std::system("open \"https://github.com/topics/pholio-plugin\"");
#else
                std::system("xdg-open \"https://github.com/topics/pholio-plugin\"");
#endif
            }

            if (m_uiPluginsEnabled && m_uiPluginManager->hasPlugins()) {
                ImGui::Separator();
                const auto descriptors = m_uiPluginManager->getPluginDescriptors();
                for (const auto& descriptor : descriptors) {
                    if (ImGui::BeginMenu(descriptor.name.c_str())) {
                        if (!descriptor.enabled) {
                            ImGui::TextDisabled("%s", i18n::tr("menu.plugin.disabled", "(disabled)"));
                            ImGui::Separator();
                        }
                        ImGui::BeginDisabled(!descriptor.enabled);
                        if (descriptor.hasWindow) {
                            if (ImGui::MenuItem(i18n::tr("menu.plugin.open_window", "Open Window / Config"))) {
                                std::string message;
                                (void)m_uiPluginManager->activatePlugin(descriptor.name, message);
                                m_logWindow->info(message);
                            }
                        } else {
                            ImGui::TextDisabled("%s", i18n::tr("menu.plugin.no_window", "No config/window provided"));
                        }
                        ImGui::EndDisabled();
                        ImGui::Separator();
                        ImGui::TextDisabled(i18n::tr("menu.plugin.version", "Version: %s"), descriptor.version.c_str());
                        ImGui::TextDisabled(i18n::tr("menu.plugin.author", "Author: %s"), descriptor.author.c_str());
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(i18n::tr("menu.help", "Help"))) {
            if (ImGui::MenuItem(i18n::tr("menu.help.check_updates", "Check for Updates"))) {
                core::UpdateManager::getInstance().checkForUpdates();
            }
            if (ImGui::MenuItem(i18n::tr("menu.help.report_issue", "Report Issue"))) m_showReport = true;
            if (ImGui::MenuItem(i18n::tr("menu.help.whats_new", "What's New"))) { 
                m_changelogWindow->loadNewEntries("0.0.0"); 
                m_showChangelog = true; 
            }
            if (ImGui::MenuItem(i18n::tr("menu.help.about", "About"))) m_showAbout = true;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void AppWindow::applyLayoutPreset(LayoutPreset preset, ImGuiID dockspaceId, const ImVec2& workSize) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, workSize);

    ImGuiID dock_main_id = dockspaceId;
    ImGuiID dock_right_id = 0;
    ImGuiID dock_bottom_id = 0;

    switch (preset) {
        case LayoutPreset::Default:
            dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, nullptr, &dock_main_id);
            dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
            ImGui::DockBuilderDockWindow("Dashboard", dock_main_id);
            ImGui::DockBuilderDockWindow("Image Preview", dock_main_id);
            ImGui::DockBuilderDockWindow("Logs", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Settings", dock_right_id);
            ImGui::DockBuilderDockWindow("Progress & Performance", dock_right_id);
            ImGui::DockBuilderDockWindow("Internal Debug", dock_bottom_id);
            break;
        case LayoutPreset::MediaFocus:
            dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.28f, nullptr, &dock_main_id);
            dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.18f, nullptr, &dock_main_id);
            ImGui::DockBuilderDockWindow("Dashboard", dock_main_id);
            ImGui::DockBuilderDockWindow("Image Preview", dock_main_id);
            ImGui::DockBuilderDockWindow("Logs", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Settings", dock_right_id);
            ImGui::DockBuilderDockWindow("Progress & Performance", dock_right_id);
            break;
        case LayoutPreset::Monitoring:
            dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.35f, nullptr, &dock_main_id);
            dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.40f, nullptr, &dock_main_id);
            ImGui::DockBuilderDockWindow("Dashboard", dock_right_id);
            ImGui::DockBuilderDockWindow("Settings", dock_right_id);
            ImGui::DockBuilderDockWindow("Progress & Performance", dock_main_id);
            ImGui::DockBuilderDockWindow("Logs", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Internal Debug", dock_bottom_id);
            break;
    }

    ImGui::DockBuilderFinish(dockspaceId);
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
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", i18n::tr("status.running", "Status: Running"));
            } else {
                ImGui::Text("%s", i18n::tr("status.idle", "Status: Idle"));
            }
            
            if (core::UpdateManager::getInstance().isChecking()) {
                ImGui::Separator();
                ImGui::TextDisabled("%s", i18n::tr("status.checking_updates", "Checking for updates..."));
            } else if (core::UpdateManager::getInstance().getUpdateInfo().hasUpdate) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", i18n::tr("status.update_available", "Update Available!"));
            }

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

void AppWindow::setupStyle(const std::string& theme) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;

    if (theme == "light") {
        ImGui::StyleColorsLight();
        colors[ImGuiCol_WindowBg]      = ImVec4(0.96f, 0.96f, 0.97f, 1.00f);
        colors[ImGuiCol_FrameBg]       = ImVec4(0.90f, 0.91f, 0.93f, 1.00f);
        colors[ImGuiCol_Button]        = ImVec4(0.84f, 0.86f, 0.89f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.58f, 0.95f, 0.90f);
        colors[ImGuiCol_Header]        = ImVec4(0.84f, 0.86f, 0.89f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.58f, 0.95f, 0.80f);
    } else {
        ImGui::StyleColorsDark();
        colors[ImGuiCol_Text]          = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_WindowBg]      = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_Border]        = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBg]       = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
        colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]        = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    }
}

} // namespace ui
