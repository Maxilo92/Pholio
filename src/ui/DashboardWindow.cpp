#include "DashboardWindow.hpp"
#include <imgui.h>
#include "../core/ConfigManager.hpp"
#include <nfd.hpp>
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace ui {

DashboardWindow::DashboardWindow(engine::Worker& worker) : m_worker(worker) {
    NFD_Init();
}

void DashboardWindow::render() {
    if (ImGui::Begin("Dashboard")) {
        renderFolderSelection();
        ImGui::Separator();
        renderControls();
        ImGui::Separator();
        renderStatus();
    }
    ImGui::End();
}

void DashboardWindow::renderFolderSelection() {
    auto settings = core::ConfigManager::getInstance().getSettings();
    bool changed = false;

    ImGui::Text("Source Folder:");
    char sourceBuf[1024];
    std::string sourceStr = settings.sourcePath.string();
    std::strncpy(sourceBuf, sourceStr.c_str(), sizeof(sourceBuf));
    if (ImGui::InputText("##Source", sourceBuf, sizeof(sourceBuf))) {
        settings.sourcePath = sourceBuf;
        changed = true;
    }
    if (ImGui::BeginPopupContextItem("SourceContextMenu")) {
        if (ImGui::MenuItem("Open in Finder/Explorer")) {
            openFolderInExplorer(settings.sourcePath);
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse...##Source")) {
        std::string path = browseFolder(sourceStr);
        if (!path.empty()) {
            settings.sourcePath = path;
            changed = true;
        }
    }

    ImGui::Text("Target Folder:");
    char targetBuf[1024];
    std::string targetStr = settings.targetPath.string();
    std::strncpy(targetBuf, targetStr.c_str(), sizeof(targetBuf));
    if (ImGui::InputText("##Target", targetBuf, sizeof(targetBuf))) {
        settings.targetPath = targetBuf;
        changed = true;
    }
    if (ImGui::BeginPopupContextItem("TargetContextMenu")) {
        if (ImGui::MenuItem("Open in Finder/Explorer")) {
            openFolderInExplorer(settings.targetPath);
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse...##Target")) {
        std::string path = browseFolder(targetStr);
        if (!path.empty()) {
            settings.targetPath = path;
            changed = true;
        }
    }

    if (changed) {
        core::ConfigManager::getInstance().setSettings(settings);
    }
}

void DashboardWindow::renderControls() {
    bool isRunning = m_worker.isRunning();

    if (isRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP PROCESS", ImVec2(-1, 50))) {
            m_worker.stop();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("START SORTING", ImVec2(-1, 50))) {
            m_worker.start();
        }
        ImGui::PopStyleColor();
    }
}

void DashboardWindow::renderStatus() {
    float progress = m_worker.getProgress();
    std::string status = m_worker.getStatusMessage();

    ImGui::Text("Status: %s", status.c_str());
    ImGui::ProgressBar(progress, ImVec2(-1, 0));

    if (m_worker.isRunning() || progress > 0.0f) {
        ImGui::Text("Files: %d / %d", m_worker.getProcessedFiles(), m_worker.getTotalFiles());
        ImGui::Text("Speed: %.2f files/sec", m_worker.getFilesPerSecond());
        
        // Simple ETA calculation
        int remaining = m_worker.getTotalFiles() - m_worker.getProcessedFiles();
        if (remaining > 0 && m_worker.getFilesPerSecond() > 0.1f) {
            int etaSeconds = static_cast<int>(remaining / m_worker.getFilesPerSecond());
            ImGui::Text("ETA: %d min %d sec", etaSeconds / 60, etaSeconds % 60);
        }
    }
}

std::string DashboardWindow::browseFolder(const std::string& defaultPath) {
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_PickFolder(&outPath, defaultPath.c_str());
    
    std::string path;
    if (result == NFD_OKAY) {
        path = outPath;
        NFD_FreePath(outPath);
    }
    
    return path;
}

void DashboardWindow::openFolderInExplorer(const std::filesystem::path& path) {
    if (path.empty() || !std::filesystem::exists(path)) return;

#ifdef _WIN32
    std::string command = "explorer \"" + path.string() + "\"";
#elif __APPLE__
    std::string command = "open \"" + path.string() + "\"";
#else
    std::string command = "xdg-open \"" + path.string() + "\"";
#endif
    std::system(command.c_str());
}

} // namespace ui
