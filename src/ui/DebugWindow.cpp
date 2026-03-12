#include "DebugWindow.hpp"
#include <imgui.h>
#include "core/UpdateManager.hpp"
#include "core/Config.hpp"
#include <iostream>

namespace ui {

void DebugWindow::render(bool* p_open) {
    if (!*p_open) return;

    if (ImGui::Begin("Internal Debug", p_open)) {
        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Performance")) {
                renderPerformance();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("API Logs")) {
                renderApiLogs();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("System Info")) {
                renderSystemInfo();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void DebugWindow::renderPerformance() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    
    // Draw simple frame time graph
    static float values[90] = { 0 };
    static int values_offset = 0;
    values[values_offset] = io.Framerate;
    values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
    
    ImGui::PlotLines("FPS", values, IM_ARRAYSIZE(values), values_offset, nullptr, 0.0f, 120.0f, ImVec2(0, 80));
}

void DebugWindow::renderApiLogs() {
    if (ImGui::Button("Clear Logs")) {
        core::UpdateManager::getInstance().clearLogs();
    }
    
    ImGui::Separator();
    
    if (ImGui::BeginChild("ApiLogsScrolling")) {
        auto logs = core::UpdateManager::getInstance().getApiLogs();
        if (logs.empty()) {
            ImGui::TextDisabled("No API calls logged yet.");
        } else {
            for (auto it = logs.rbegin(); it != logs.rend(); ++it) {
                ImGui::TextDisabled("[%s]", it->timestamp.c_str());
                ImGui::SameLine();
                ImGui::TextColored(it->statusCode == 200 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), 
                                   "Status %d", it->statusCode);
                ImGui::SameLine();
                ImGui::TextUnformatted(it->endpoint.c_str());
                
                if (ImGui::TreeNode((std::string("Response##") + it->timestamp).c_str())) {
                    ImGui::TextWrapped("%s", it->response.c_str());
                    ImGui::TreePop();
                }
                ImGui::Separator();
            }
        }
    }
    ImGui::EndChild();
}

void DebugWindow::renderSystemInfo() {
    ImGui::Text("Project: %s", core::PROJECT_NAME.data());
    ImGui::Text("Version: %s", core::PROJECT_VERSION.data());
    
#ifdef __APPLE__
    ImGui::Text("Platform: macOS");
#elif _WIN32
    ImGui::Text("Platform: Windows");
#else
    ImGui::Text("Platform: Linux/Other");
#endif

    ImGui::Separator();
    ImGui::Text("ImGui Version: %s", IMGUI_VERSION);
}

} // namespace ui
