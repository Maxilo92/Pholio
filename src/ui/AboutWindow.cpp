#include "core/Config.hpp"
#include "AboutWindow.hpp"
#include <string>

namespace ui {

void AboutWindow::render(bool* p_open) {
    if (!*p_open) return;

    std::string title = "About " + std::string(core::PROJECT_NAME);
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(title.c_str(), p_open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "%s", core::PROJECT_NAME);
        ImGui::Separator();
        
        ImGui::Text("Version: %s", core::PROJECT_VERSION);
        ImGui::Text("Author: Maximilian");
        ImGui::Text("A powerful and efficient tool for organizing your media library.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Pholio Evolution (1.0.0-alpha):");
        ImGui::BulletText("Intelligent Structure Analysis");
        ImGui::BulletText("Robust Crash Reporting");
        ImGui::BulletText("Real-time Photo Previews");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Dependencies:");
        ImGui::BulletText("ImGui - Graphical User Interface");
        ImGui::BulletText("Exiv2 - Metadata extraction");
        ImGui::BulletText("FFmpeg - Video metadata & frames");
        ImGui::BulletText("backward-cpp - Crash analysis");
        
        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            *p_open = false;
        }
    }
    ImGui::End();
}

} // namespace ui
