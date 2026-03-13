#include "DashboardWindow.hpp"
#include <imgui.h>
#include "../core/ConfigManager.hpp"
#include <nfd.hpp>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <cstdint>

namespace ui {

namespace {
std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f %s", size, units[unit]);
    return std::string(buf);
}

std::string formatDuration(int64_t seconds) {
    const int64_t hrs = seconds / 3600;
    const int64_t mins = (seconds % 3600) / 60;
    const int64_t secs = seconds % 60;
    char buf[64];
    if (hrs > 0) {
        std::snprintf(buf, sizeof(buf), "%lldh %lldm %llds",
                      static_cast<long long>(hrs), static_cast<long long>(mins), static_cast<long long>(secs));
    } else {
        std::snprintf(buf, sizeof(buf), "%lldm %llds",
                      static_cast<long long>(mins), static_cast<long long>(secs));
    }
    return std::string(buf);
}
} // namespace

DashboardWindow::DashboardWindow(engine::Worker& worker) : m_worker(worker) {
    NFD_Init();
}

void DashboardWindow::render() {
    auto settings = core::ConfigManager::getInstance().getSettings();
    std::string status = m_worker.getStatusMessage();
    bool isRunning = m_worker.isRunning();

    // Summary detection logic
    if (m_workerWasRunning && !isRunning) {
        if (status == "Completed") {
            m_showSummaryPopup = true;
        } else if (status.find("Error") != std::string::npos || status.find("Full") != std::string::npos) {
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
    renderSummaryPopup();
    renderDuplicatePopup();

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

void DashboardWindow::renderDuplicatePreviewPanel(const char* label, const std::filesystem::path& imagePath, Texture& texture, std::filesystem::path& lastLoadedPath) {
    ImGui::BeginChild(label, ImVec2(0, 280), true);
    ImGui::TextWrapped("%s", label);
    ImGui::Separator();
    ImGui::TextWrapped("Name: %s", imagePath.filename().string().c_str());
    ImGui::TextWrapped("Path: %s", imagePath.string().c_str());
    ImGui::Spacing();

    if (imagePath != lastLoadedPath) {
        if (texture.loadFromFile(imagePath)) {
            lastLoadedPath = imagePath;
        } else {
            texture.release();
            lastLoadedPath.clear();
        }
    }

    if (texture.isValid()) {
        const float maxWidth = ImGui::GetContentRegionAvail().x;
        const float maxHeight = 170.0f;
        const float texWidth = static_cast<float>(texture.getWidth());
        const float texHeight = static_cast<float>(texture.getHeight());
        float displayWidth = maxWidth;
        float displayHeight = displayWidth * (texHeight / texWidth);
        if (displayHeight > maxHeight) {
            displayHeight = maxHeight;
            displayWidth = displayHeight * (texWidth / texHeight);
        }
        ImTextureID texID = (ImTextureID)(intptr_t)texture.getID();
        ImGui::Image(texID, ImVec2(displayWidth, displayHeight));
    } else {
        ImGui::TextWrapped("Preview not available for this file.");
    }

    ImGui::EndChild();
}

void DashboardWindow::renderDuplicatePopup() {
    auto prompt = m_worker.getPendingDuplicatePrompt();
    if (prompt && !m_duplicatePopupOpenRequested) {
        ImGui::OpenPopup("Duplicate Found");
        m_duplicatePopupOpenRequested = true;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(980.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Duplicate Found", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!prompt) {
            m_duplicatePopupOpenRequested = false;
            m_duplicateSourceTexture.release();
            m_duplicateTargetTexture.release();
            m_lastDuplicateSourceLoadedPath.clear();
            m_lastDuplicateTargetLoadedPath.clear();
            m_applyDecisionToRemainingDuplicates = false;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        ImGui::TextWrapped("A duplicate was found in the target location. Compare both files and choose how to proceed.");
        ImGui::Spacing();

        if (ImGui::BeginTable("DuplicateCompareTable", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            renderDuplicatePreviewPanel("Incoming file", prompt->sourcePath, m_duplicateSourceTexture, m_lastDuplicateSourceLoadedPath);
            ImGui::TableNextColumn();
            renderDuplicatePreviewPanel("Existing target file", prompt->targetPath, m_duplicateTargetTexture, m_lastDuplicateTargetLoadedPath);
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Checkbox("Apply selected action to all remaining duplicates in this run", &m_applyDecisionToRemainingDuplicates);
        ImGui::Spacing();
        if (ImGui::Button("Skip", ImVec2(140, 0))) {
            m_worker.submitDuplicateDecision(engine::DuplicateAction::Skip, m_applyDecisionToRemainingDuplicates);
            m_applyDecisionToRemainingDuplicates = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Rename Incoming", ImVec2(160, 0))) {
            m_worker.submitDuplicateDecision(engine::DuplicateAction::Rename, m_applyDecisionToRemainingDuplicates);
            m_applyDecisionToRemainingDuplicates = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Overwrite Existing", ImVec2(170, 0))) {
            m_worker.submitDuplicateDecision(engine::DuplicateAction::Overwrite, m_applyDecisionToRemainingDuplicates);
            m_applyDecisionToRemainingDuplicates = false;
        }

        ImGui::EndPopup();
    } else if (!prompt) {
        m_duplicatePopupOpenRequested = false;
        m_duplicateSourceTexture.release();
        m_duplicateTargetTexture.release();
        m_lastDuplicateSourceLoadedPath.clear();
        m_lastDuplicateTargetLoadedPath.clear();
        m_applyDecisionToRemainingDuplicates = false;
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

void DashboardWindow::renderSummaryPopup() {
    if (m_showSummaryPopup) {
        ImGui::OpenPopup("Process Finished");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Process Finished", &m_showSummaryPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("The media organization process has finished.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTable("SummaryTable", 2)) {
            const int total = m_worker.getTotalFiles();
            const int success = m_worker.getSuccessCount();
            const int errors = m_worker.getErrorCount();
            const uint64_t bytes = m_worker.getProcessedBytes();
            const int64_t durationSec = m_worker.getLastRunDurationSeconds();
            const float successRate = total > 0 ? (100.0f * static_cast<float>(success) / static_cast<float>(total)) : 0.0f;
            const float avgFilesPerSec = durationSec > 0 ? static_cast<float>(m_worker.getProcessedFiles()) / static_cast<float>(durationSec) : 0.0f;
            const float avgMBPerSec = durationSec > 0 ? (static_cast<float>(bytes) / (1024.0f * 1024.0f)) / static_cast<float>(durationSec) : 0.0f;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Total Files:");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", total);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Successful:");
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", success);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Failed/Skipped:");
            ImGui::TableSetColumnIndex(1); 
            if (errors > 0)
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%d", errors);
            else
                ImGui::Text("%d", errors);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Success Rate:");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f%%", successRate);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Data Processed:");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", formatBytes(bytes).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Duration:");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", formatDuration(durationSec).c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("Avg Speed:");
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f files/s (%.2f MB/s)", avgFilesPerSec, avgMBPerSec);

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Great!", ImVec2(120, 0))) {
            m_showSummaryPopup = false;
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
