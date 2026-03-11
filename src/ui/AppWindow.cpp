#include "AppWindow.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include "../core/ConfigManager.hpp"

namespace ui {

AppWindow::AppWindow() 
    : m_logWindow(std::make_unique<LogWindow>()),
      m_worker(std::make_unique<engine::Worker>(*m_logWindow)),
      m_settingsWindow(std::make_unique<SettingsWindow>()),
      m_progressWindow(std::make_unique<ProgressWindow>(*m_worker)),
      m_dashboardWindow(std::make_unique<DashboardWindow>(*m_worker)),
      m_aboutWindow(std::make_unique<AboutWindow>()) {
    
    // Setup file logging
    m_logWindow->setupFileLogging(core::ConfigManager::getInstance().getLogDirectory());
}

AppWindow::~AppWindow() = default;

void AppWindow::update() {
    // Logic updates if needed
}

void AppWindow::render() {
    renderMainDockspace();

    if (m_showDashboard) {
        m_dashboardWindow->render();
    }

    if (m_showSettings) {
        m_settingsWindow->render();
    }
    
    if (m_showProgress) {
        m_progressWindow->render();
    }

    if (m_showLogs) {
        m_logWindow->render();
    }

    if (m_showAbout) {
        m_aboutWindow->render(&m_showAbout);
    }
}

void AppWindow::renderMainDockspace() {
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("MainDockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Source Folder...")) {
                m_showDashboard = true;
                // We could trigger the browse here if we wanted
            }
            if (ImGui::MenuItem("Open Target Folder...")) {
                m_showDashboard = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // The main loop handles this if we set a flag, 
                // but for now we'll just use the window close button.
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Dashboard", nullptr, &m_showDashboard);
            ImGui::MenuItem("Settings", nullptr, &m_showSettings);
            ImGui::MenuItem("Progress & Performance", nullptr, &m_showProgress);
            ImGui::MenuItem("Logs", nullptr, &m_showLogs);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockBuilderRemoveNode(dockspace_id);
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
                
                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, NULL, &dock_main_id);
                ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, NULL, &dock_main_id);
                
                ImGui::DockBuilderDockWindow("Dashboard", dock_main_id);
                ImGui::DockBuilderDockWindow("Logs", dock_id_bottom);
                ImGui::DockBuilderDockWindow("Settings", dock_id_right);
                ImGui::DockBuilderDockWindow("Progress & Performance", dock_id_right);
                
                ImGui::DockBuilderFinish(dockspace_id);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About PhotoSorter")) {
                m_showAbout = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

} // namespace ui
