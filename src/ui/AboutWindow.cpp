#include "AboutWindow.hpp"

namespace ui {

void AboutWindow::render(bool* p_open) {
    if (!*p_open) return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("About PhotoSorter", p_open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "PhotoSorter C++");
        ImGui::Separator();
        
        ImGui::Text("Version: 0.1.0");
        ImGui::Text("Author: Maximilian");
        ImGui::Text("A powerful and efficient tool for organizing your media library.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Dependencies:");
        ImGui::BulletText("ImGui - Graphical User Interface");
        ImGui::BulletText("Exiv2 - Metadata extraction");
        ImGui::BulletText("FFmpeg - Media decoding");
        ImGui::BulletText("nfd-extended - Native file dialogs");
        
        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            *p_open = false;
        }
    }
    ImGui::End();
}

} // namespace ui
