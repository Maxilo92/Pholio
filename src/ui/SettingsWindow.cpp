#include "SettingsWindow.hpp"
#include <imgui.h>
#include <nfd.hpp>
#include <cstring>
#include <string>

namespace ui {

void SettingsWindow::render() {
    auto& config = core::ConfigManager::getInstance();
    
    // Sync from global config if we aren't currently editing something unsaved
    if (!m_initialized || !m_isDirty) {
        m_editedSettings = config.getSettings();
        m_initialized = true;
    }

    if (ImGui::Begin("Settings")) {
        if (m_isDirty) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "You have unsaved changes!");
            ImGui::Separator();
        }

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
                std::strncpy(sourceBuf, m_editedSettings.sourcePath.string().c_str(), sizeof(sourceBuf));
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::InputText("##SourceSet", sourceBuf, sizeof(sourceBuf))) {
                    m_editedSettings.sourcePath = sourceBuf;
                    m_isDirty = true;
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
                std::strncpy(targetBuf, m_editedSettings.targetPath.string().c_str(), sizeof(targetBuf));
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::InputText("##TargetSet", targetBuf, sizeof(targetBuf))) {
                    m_editedSettings.targetPath = targetBuf;
                    m_isDirty = true;
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
                int opMode = static_cast<int>(m_editedSettings.operationMode);
                const char* opModes[] = { "Copy (Safe)", "Move (Efficient)" };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##OpMode", &opMode, opModes, 2)) {
                    m_editedSettings.operationMode = static_cast<engine::OperationMode>(opMode);
                    m_isDirty = true;
                }
                ImGui::PopItemWidth();

                // Verif Level
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Verification:");
                ImGui::TableSetColumnIndex(1);
                int verLevel = static_cast<int>(m_editedSettings.verificationLevel);
                const char* verLevels[] = { "None (Fastest)", "Size Only", "Partial Hash", "Full Hash (Safest)" };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##VerLevel", &verLevel, verLevels, 4)) {
                    m_editedSettings.verificationLevel = static_cast<engine::VerificationLevel>(verLevel);
                    m_isDirty = true;
                }
                ImGui::PopItemWidth();

                ImGui::EndTable();
            }

            ImGui::Spacing();
            if (ImGui::Checkbox("Dry Run (Simulation Mode)", &m_editedSettings.dryRun)) m_isDirty = true;
            if (ImGui::Checkbox("Auto-Start on selection", &m_editedSettings.autoStart)) m_isDirty = true;
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("SAVE ALL SETTINGS", ImVec2(150, 40))) {
            config.setSettings(m_editedSettings);
            config.save();
            m_isDirty = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("DISCARD CHANGES", ImVec2(150, 40))) {
            m_editedSettings = config.getSettings();
            m_isDirty = false;
        }
    }
    ImGui::End();

    // Deferred browsing
    if (m_shouldBrowseSource || m_shouldBrowseTarget) {
        nfdchar_t *outPath = NULL;
        std::string defaultPath = m_shouldBrowseSource ? m_editedSettings.sourcePath.string() : m_editedSettings.targetPath.string();
        
        if (NFD_PickFolder(&outPath, defaultPath.c_str()) == NFD_OKAY) {
            if (m_shouldBrowseSource) {
                m_editedSettings.sourcePath = outPath;
            } else {
                m_editedSettings.targetPath = outPath;
            }
            m_isDirty = true;
            NFD_FreePath(outPath);
        }
        
        m_shouldBrowseSource = false;
        m_shouldBrowseTarget = false;
    }
}

} // namespace ui
