#include "PreviewWindow.hpp"
#include <imgui.h>
#include <iostream>

namespace ui {

PreviewWindow::PreviewWindow(engine::Worker& worker) : m_worker(worker) {}

void PreviewWindow::setExternalImagePath(const std::filesystem::path& path) {
    m_externalImagePath = path;
}

void PreviewWindow::clearExternalImagePath() {
    m_externalImagePath.reset();
}

void PreviewWindow::render(bool* p_open) {
    if (!*p_open) return;

    if (ImGui::Begin("Image Preview", p_open)) {
        std::filesystem::path currentPath = m_externalImagePath.has_value()
            ? *m_externalImagePath
            : m_worker.getCurrentImagePath();
        
        if (!currentPath.empty() && currentPath != m_lastLoadedPath) {
            if (m_previewTexture.loadFromFile(currentPath)) {
                m_lastLoadedPath = currentPath;
            } else {
                m_previewTexture.release();
                m_lastLoadedPath = currentPath;
            }
        } else if (currentPath.empty()) {
            m_previewTexture.release();
            m_lastLoadedPath = "";
        }

        if (m_previewTexture.isValid()) {
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float windowHeight = ImGui::GetContentRegionAvail().y - 20.0f; // Leave space for text
            
            float texWidth = static_cast<float>(m_previewTexture.getWidth());
            float texHeight = static_cast<float>(m_previewTexture.getHeight());
            
            float aspectRatio = texHeight / texWidth;
            float displayWidth = windowWidth;
            float displayHeight = displayWidth * aspectRatio;
            
            if (displayHeight > windowHeight) {
                displayHeight = windowHeight;
                displayWidth = displayHeight / aspectRatio;
            }

            if (m_externalImagePath.has_value()) {
                ImGui::Text("Gallery File: %s", currentPath.filename().string().c_str());
            } else {
                ImGui::Text("Current File: %s", currentPath.filename().string().c_str());
            }
            ImTextureID texID = (ImTextureID)(intptr_t)m_previewTexture.getID();
            ImGui::Image(texID, ImVec2(displayWidth, displayHeight));
        } else {
            if (currentPath.empty()) {
                if (m_externalImagePath.has_value()) {
                    ImGui::Text("No gallery image selected.");
                } else {
                    ImGui::Text("No preview available. Start sorting to see images.");
                }
            } else {
                ImGui::Text("No preview available for file:");
                ImGui::TextWrapped("%s", currentPath.filename().string().c_str());
                ImGui::Spacing();
                ImGui::TextDisabled("Path:");
                ImGui::TextWrapped("%s", currentPath.string().c_str());
            }
        }
    }
    ImGui::End();
}

} // namespace ui
