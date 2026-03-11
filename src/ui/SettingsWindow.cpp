#include "SettingsWindow.hpp"
#include <imgui.h>
#include "../core/ConfigManager.hpp"
#include <nfd.hpp>
#include <cstring>
#include <string>

namespace ui {

void SettingsWindow::render() {
    static core::AppSettings settings = core::ConfigManager::getInstance().getSettings();
    static bool first_run = true;
    if (first_run) {
        settings = core::ConfigManager::getInstance().getSettings();
        first_run = false;
    }

    if (ImGui::Begin("Settings")) {
        // Paths
        char sourceBuf[1024];
        std::string sourceStr = settings.sourcePath.string();
        std::strncpy(sourceBuf, sourceStr.c_str(), sizeof(sourceBuf));
        sourceBuf[sizeof(sourceBuf) - 1] = '\0';
        if (ImGui::InputText("Source Directory", sourceBuf, sizeof(sourceBuf))) {
            settings.sourcePath = sourceBuf;
        }
        ImGui::SetItemTooltip("The directory where your unorganized photos and videos are located.");
        
        ImGui::SameLine();
        if (ImGui::Button("Browse...##SourceSet")) {
            m_shouldBrowseSource = true;
        }

        char targetBuf[1024];
        std::string targetStr = settings.targetPath.string();
        std::strncpy(targetBuf, targetStr.c_str(), sizeof(targetBuf));
        targetBuf[sizeof(targetBuf) - 1] = '\0';
        if (ImGui::InputText("Target Directory", targetBuf, sizeof(targetBuf))) {
            settings.targetPath = targetBuf;
        }
        ImGui::SetItemTooltip("The root directory where the organized library will be created.");

        ImGui::SameLine();
        if (ImGui::Button("Browse...##TargetSet")) {
            m_shouldBrowseTarget = true;
        }

        ImGui::Separator();

        // Operation Mode
        int opMode = static_cast<int>(settings.operationMode);
        const char* opModes[] = { "Copy", "Move" };
        if (ImGui::Combo("Operation Mode", &opMode, opModes, 2)) {
            settings.operationMode = static_cast<engine::OperationMode>(opMode);
        }
        ImGui::SetItemTooltip("Copy: Keep original files.\nMove: Transfer files to new location (deletes originals).");

        // Verification Level
        int verLevel = static_cast<int>(settings.verificationLevel);
        const char* verLevels[] = { "None", "SizeOnly", "Partial", "Full" };
        if (ImGui::Combo("Verification Level", &verLevel, verLevels, 4)) {
            settings.verificationLevel = static_cast<engine::VerificationLevel>(verLevel);
        }
        ImGui::SetItemTooltip("None: No check.\nSizeOnly: Check file size.\nPartial: Check first 1MB hash.\nFull: Check entire file hash.");

        ImGui::Separator();

        ImGui::Checkbox("Dry Run", &settings.dryRun);
        ImGui::SetItemTooltip("Simulate the process without actually moving or copying any files.");
        
        ImGui::Checkbox("Auto Start", &settings.autoStart);
        ImGui::SetItemTooltip("Automatically start the sorting process when a valid source and target are selected.");

        ImGui::Separator();

        if (ImGui::Button("Save Settings")) {
            core::ConfigManager::getInstance().setSettings(settings);
            core::ConfigManager::getInstance().save();
        }
        ImGui::SetItemTooltip("Apply and persist these settings to disk.");
        
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            settings = core::ConfigManager::getInstance().getSettings();
        }
        ImGui::SetItemTooltip("Discard changes and reload last saved settings.");
    }
    ImGui::End();

    // Deferred browsing
    if (m_shouldBrowseSource || m_shouldBrowseTarget) {
        nfdchar_t *outPath = NULL;
        std::string defaultPath = m_shouldBrowseSource ? settings.sourcePath.string() : settings.targetPath.string();
        
        if (NFD_PickFolder(&outPath, defaultPath.c_str()) == NFD_OKAY) {
            if (m_shouldBrowseSource) {
                settings.sourcePath = outPath;
            } else {
                settings.targetPath = outPath;
            }
            NFD_FreePath(outPath);
        }
        
        m_shouldBrowseSource = false;
        m_shouldBrowseTarget = false;
    }
}

} // namespace ui
