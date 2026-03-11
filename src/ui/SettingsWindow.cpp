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
        if (ImGui::CollapsingHeader("Directories", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("DirectorySettings", 3, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.0f);

                // Source
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Source Path:");
                ImGui::TableSetColumnIndex(1);
                char sourceBuf[1024];
                std::strncpy(sourceBuf, settings.sourcePath.string().c_str(), sizeof(sourceBuf));
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::InputText("##SourceSet", sourceBuf, sizeof(sourceBuf))) {
                    settings.sourcePath = sourceBuf;
                }
                ImGui::PopItemWidth();
                ImGui::TableSetColumnIndex(2);
                if (ImGui::Button("Browse...##SourceSet")) m_shouldBrowseSource = true;

                // Target
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Target Path:");
                ImGui::TableSetColumnIndex(1);
                char targetBuf[1024];
                std::strncpy(targetBuf, settings.targetPath.string().c_str(), sizeof(targetBuf));
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::InputText("##TargetSet", targetBuf, sizeof(targetBuf))) {
                    settings.targetPath = targetBuf;
                }
                ImGui::PopItemWidth();
                ImGui::TableSetColumnIndex(2);
                if (ImGui::Button("Browse...##TargetSet")) m_shouldBrowseTarget = true;

                ImGui::EndTable();
            }
        }

        if (ImGui::CollapsingHeader("Engine Behavior", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("EngineSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

                // Op Mode
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Operation Mode:");
                ImGui::TableSetColumnIndex(1);
                int opMode = static_cast<int>(settings.operationMode);
                const char* opModes[] = { "Copy (Safe)", "Move (Efficient)" };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##OpMode", &opMode, opModes, 2)) {
                    settings.operationMode = static_cast<engine::OperationMode>(opMode);
                }
                ImGui::PopItemWidth();

                // Verif Level
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Verification:");
                ImGui::TableSetColumnIndex(1);
                int verLevel = static_cast<int>(settings.verificationLevel);
                const char* verLevels[] = { "None (Fastest)", "Size Only", "Partial Hash", "Full Hash (Safest)" };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##VerLevel", &verLevel, verLevels, 4)) {
                    settings.verificationLevel = static_cast<engine::VerificationLevel>(verLevel);
                }
                ImGui::PopItemWidth();

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::Checkbox("Dry Run (Simulation Mode)", &settings.dryRun);
            ImGui::Checkbox("Auto-Start on selection", &settings.autoStart);
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("SAVE ALL SETTINGS", ImVec2(150, 40))) {
            core::ConfigManager::getInstance().setSettings(settings);
            core::ConfigManager::getInstance().save();
        }
        ImGui::SameLine();
        if (ImGui::Button("DISCARD CHANGES", ImVec2(150, 40))) {
            settings = core::ConfigManager::getInstance().getSettings();
        }
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
