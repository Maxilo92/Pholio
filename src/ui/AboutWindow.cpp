#include "core/Config.hpp"
#include "AboutWindow.hpp"
#include "core/UpdateManager.hpp"
#include <string>

namespace ui {

void AboutWindow::render(bool* p_open) {
    if (!*p_open) return;

    std::string title = "About " + std::string(core::PROJECT_NAME);
    ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(title.c_str(), p_open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "%.*s", (int)core::PROJECT_NAME.size(), core::PROJECT_NAME.data());
        ImGui::Separator();
        
        ImGui::Text("Version: %.*s", (int)core::PROJECT_VERSION.size(), core::PROJECT_VERSION.data());
        ImGui::Text("Author: Maximilian");
        ImGui::Text("A powerful and efficient tool for organizing your media library.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Core Features:");
        ImGui::BulletText("Intelligent Structure Analysis");
        ImGui::BulletText("Robust Crash Reporting");
        ImGui::BulletText("Real-time Photo Previews");
        ImGui::BulletText("Google Photos Takeout Integration");
        ImGui::BulletText("macOS App Bundle Support");
        ImGui::BulletText("User Reporting System (via Help)");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Dependencies:");
        ImGui::BulletText("ImGui - Graphical User Interface");
        ImGui::BulletText("Exiv2 - Metadata extraction");
        ImGui::BulletText("FFmpeg - Video metadata & frames");
        ImGui::BulletText("backward-cpp - Crash analysis");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (core::UpdateManager::getInstance().isChecking()) {
            ImGui::Button("Checking for Updates...", ImVec2(180, 0));
        } else {
            if (ImGui::Button("Check for Update", ImVec2(180, 0))) {
                core::UpdateManager::getInstance().checkForUpdates();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(180, 0))) {
            *p_open = false;
        }
    }
    ImGui::End();
}

} // namespace ui
