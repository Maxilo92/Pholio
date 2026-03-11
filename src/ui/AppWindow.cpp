#include "AppWindow.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include "core/ConfigManager.hpp"
#include "core/Config.hpp"

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

    // Apply high-quality style
    setupStyle();
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

    renderStatusBar();
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
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    // Initial Layout Setup
    if (m_firstRun || ImGui::DockBuilderGetNode(dockspace_id) == NULL) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
        
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.30f, NULL, &dock_main_id);
        ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, NULL, &dock_main_id);
        
        ImGui::DockBuilderDockWindow("Dashboard", dock_main_id);
        ImGui::DockBuilderDockWindow("Logs", dock_id_bottom);
        ImGui::DockBuilderDockWindow("Settings", dock_id_right);
        ImGui::DockBuilderDockWindow("Progress & Performance", dock_id_right);
        
        ImGui::DockBuilderFinish(dockspace_id);
        m_firstRun = false;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Dashboard", "Ctrl+D", m_showDashboard)) {
                m_showDashboard = true;
            }
            if (ImGui::MenuItem("Settings", "Ctrl+S", m_showSettings)) {
                m_showSettings = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Restart", "Ctrl+R")) {
                m_shouldRestart = true;
            }
            if (ImGui::MenuItem("Rebuild & Restart", "Ctrl+Shift+R")) {
                m_shouldRebuild = true;
                m_shouldRestart = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Potential signal to main loop
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
                m_firstRun = true;
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

void AppWindow::renderStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeightWithSpacing()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeightWithSpacing()));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("StatusBar", nullptr, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            ImGui::Text("PhotoSorter v0.4.3");
            ImGui::Separator();
            
            bool isRunning = m_worker->isRunning();
            if (isRunning) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: Running");
                ImGui::Separator();
                ImGui::Text("Speed: %.2f fps", m_worker->getFilesPerSecond());
            } else {
                ImGui::Text("Status: Idle");
            }
            
            ImGui::Separator();
            auto settings = core::ConfigManager::getInstance().getSettings();
            ImGui::Text("Mode: %s", settings.operationMode == engine::OperationMode::Copy ? "Copy" : "Move");
            
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

void AppWindow::setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    // Modern Dark Theme
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.15f, 0.15f, 0.17f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.14f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.31f, 0.33f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.15f, 0.15f, 0.17f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.10f, 0.10f, 0.12f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

} // namespace ui
