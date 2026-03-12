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
    auto settings = core::ConfigManager::getInstance().getSettings();
    std::string status = m_worker.getStatusMessage();
    bool isRunning = m_worker.isRunning();

    // Error detection logic
    if (m_workerWasRunning && !isRunning) {
        if (status.find("Error") != std::string::npos || status.find("Full") != std::string::npos) {
            m_lastErrorMessage = status;
            m_showErrorPopup = true;
        }
    }
    m_workerWasRunning = isRunning;

    if (ImGui::Begin("Dashboard")) {
        renderFolderSelection();
        ImGui::Separator();
        renderControls();
        ImGui::Separator();
        renderStatus();
    }
    ImGui::End();

    renderErrorPopup();

    // Handle deferred browsing
    if (m_shouldBrowseSource || m_shouldBrowseTarget) {
        auto currentSettings = core::ConfigManager::getInstance().getSettings();
        bool changed = false;

        if (m_shouldBrowseSource) {
            std::string path = browseFolder(currentSettings.sourcePath.string());
            if (!path.empty()) {
                currentSettings.sourcePath = path;
                changed = true;
            }
            m_shouldBrowseSource = false;
        } else if (m_shouldBrowseTarget) {
            std::string path = browseFolder(currentSettings.targetPath.string());
            if (!path.empty()) {
                currentSettings.targetPath = path;
                changed = true;
            }
            m_shouldBrowseTarget = false;
        }

        if (changed) {
            core::ConfigManager::getInstance().setSettings(currentSettings);
            core::ConfigManager::getInstance().save();
        }
    }
}

void DashboardWindow::renderErrorPopup() {
    if (m_showErrorPopup) {
        ImGui::OpenPopup("Critical Error");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Critical Error", &m_showErrorPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "An error occurred during processing:");
        ImGui::Spacing();
        ImGui::TextWrapped("%s", m_lastErrorMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            m_showErrorPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DashboardWindow::renderFolderSelection() {
    auto settings = core::ConfigManager::getInstance().getSettings();
    bool changed = false;

    if (ImGui::BeginTable("FolderSelection", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.0f);

        // Source Folder
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Source:");
        
        ImGui::TableSetColumnIndex(1);
        char sourceBuf[1024];
        std::string sourceStr = settings.sourcePath.string();
        std::strncpy(sourceBuf, sourceStr.c_str(), sizeof(sourceBuf));
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputText("##Source", sourceBuf, sizeof(sourceBuf))) {
            settings.sourcePath = sourceBuf;
            changed = true;
        }
        ImGui::PopItemWidth();
        if (ImGui::BeginPopupContextItem("SourceContextMenu")) {
            if (ImGui::MenuItem("Open in Finder/Explorer")) {
                openFolderInExplorer(settings.sourcePath);
            }
            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(2);
        if (ImGui::Button("Browse...##Source")) {
            m_shouldBrowseSource = true;
        }

        // Target Folder
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Target:");
        
        ImGui::TableSetColumnIndex(1);
        char targetBuf[1024];
        std::string targetStr = settings.targetPath.string();
        std::strncpy(targetBuf, targetStr.c_str(), sizeof(targetBuf));
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputText("##Target", targetBuf, sizeof(targetBuf))) {
            settings.targetPath = targetBuf;
            changed = true;
        }
        ImGui::PopItemWidth();
        if (ImGui::BeginPopupContextItem("TargetContextMenu")) {
            if (ImGui::MenuItem("Open in Finder/Explorer")) {
                openFolderInExplorer(settings.targetPath);
            }
            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(2);
        if (ImGui::Button("Browse...##Target")) {
            m_shouldBrowseTarget = true;
        }

        ImGui::EndTable();
    }

    if (changed) {
        core::ConfigManager::getInstance().setSettings(settings);
        core::ConfigManager::getInstance().save();
    }
}

void DashboardWindow::renderControls() {
    bool isRunning = m_worker.isRunning();
    float buttonHeight = 60.0f;

    ImGui::Spacing();
    if (isRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button("STOP PROCESS", ImVec2(-1, buttonHeight))) {
            m_worker.stop();
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.4f, 0.0f, 1.0f));
        if (ImGui::Button("START SORTING", ImVec2(-1, buttonHeight))) {
            m_worker.start();
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::Spacing();
}

void DashboardWindow::renderStatus() {
    float progress = m_worker.getProgress();
    std::string status = m_worker.getStatusMessage();
    bool isRunning = m_worker.isRunning();

    ImGui::BeginGroup();
    ImGui::Text("Overall Progress:");
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Text("%.1f%%", progress * 100.0f);
    
    ImGui::ProgressBar(progress, ImVec2(-1, 15), "");
    ImGui::EndGroup();

    ImGui::Spacing();
    
    if (ImGui::BeginChild("StatusDetails", ImVec2(0, 120), true)) {
        ImGui::Columns(2, "StatusColumns", false);
        ImGui::SetColumnWidth(0, 150.0f);

        ImGui::Text("Current Status:"); ImGui::NextColumn();
        if (status.find("Error") != std::string::npos || status.find("Full") != std::string::npos) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", status.c_str());
        } else {
            ImGui::Text("%s", status.c_str());
        }
        ImGui::NextColumn();

        if (isRunning || progress > 0.0f) {
            ImGui::Text("Data Processed:"); ImGui::NextColumn();
            ImGui::Text("%.2f / %.2f GB", 
                       static_cast<double>(m_worker.getProcessedBytes()) / (1024.0 * 1024.0 * 1024.0),
                       static_cast<double>(m_worker.getTotalBytes()) / (1024.0 * 1024.0 * 1024.0)); ImGui::NextColumn();

            ImGui::Text("Files Processed:"); ImGui::NextColumn();
            ImGui::Text("%d / %d", m_worker.getProcessedFiles(), m_worker.getTotalFiles()); ImGui::NextColumn();

            ImGui::Text("Processing Speed:"); ImGui::NextColumn();
            float bytesPerSec = m_worker.getBytesPerSecond();
            if (bytesPerSec > 1024.0f * 1024.0f * 1024.0f) {
                ImGui::Text("%.2f files/sec (%.2f GB/s)", m_worker.getFilesPerSecond(), bytesPerSec / (1024.0 * 1024.0 * 1024.0));
            } else {
                ImGui::Text("%.2f files/sec (%.2f MB/s)", m_worker.getFilesPerSecond(), bytesPerSec / (1024.0 * 1024.0));
            }
            ImGui::NextColumn();

            int remaining = m_worker.getTotalFiles() - m_worker.getProcessedFiles();
            if (remaining > 0 && m_worker.getFilesPerSecond() > 0.1f) {
                int etaSeconds = static_cast<int>(remaining / m_worker.getFilesPerSecond());
                ImGui::Text("Estimated Time:"); ImGui::NextColumn();
                ImGui::Text("%d min %d sec", etaSeconds / 60, etaSeconds % 60); ImGui::NextColumn();
            }
        }
        ImGui::Columns(1);
    }
    ImGui::EndChild();
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
