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
                ImGui::SetItemTooltip("The directory where your unorganized photos and videos are located.");
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
                ImGui::SetItemTooltip("The root directory where the organized library will be created.");
                ImGui::PopItemWidth();
                ImGui::TableSetColumnIndex(2);
                if (ImGui::Button("Browse...##TargetSet")) m_shouldBrowseTarget = true;

                ImGui::EndTable();
            }

            // Source Size Calculation
            if (!m_editedSettings.sourcePath.empty() && std::filesystem::exists(m_editedSettings.sourcePath)) {
                if (m_lastCalculatedSourcePath != m_editedSettings.sourcePath && !m_isCalculatingSourceSize) {
                    m_isCalculatingSourceSize = true;
                    m_lastCalculatedSourcePath = m_editedSettings.sourcePath;
                    m_sourceSizeFuture = std::async(std::launch::async, [path = m_editedSettings.sourcePath]() {
                        uint64_t totalSize = 0;
                        try {
                            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                                if (entry.is_regular_file()) {
                                    totalSize += entry.file_size();
                                }
                            }
                        } catch (...) {}
                        return totalSize;
                    });
                }

                if (m_isCalculatingSourceSize && m_sourceSizeFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    m_sourceSize = m_sourceSizeFuture.get();
                    m_isCalculatingSourceSize = false;
                }

                ImGui::Text("Source Size: %.2f GB %s", 
                    static_cast<double>(m_sourceSize) / (1024.0 * 1024.0 * 1024.0),
                    m_isCalculatingSourceSize ? "(calculating...)" : "");
            }

            // Disk Space Warning Bar
            if (!m_editedSettings.targetPath.empty() && std::filesystem::exists(m_editedSettings.targetPath)) {
                try {
                    auto space = std::filesystem::space(m_editedSettings.targetPath);
                    uint64_t capacity = space.capacity;
                    uint64_t available = space.available;
                    uint64_t used = capacity - available;
                    
                    float usedRatio = static_cast<float>(used) / static_cast<float>(capacity);
                    float requiredRatio = static_cast<float>(m_sourceSize) / static_cast<float>(available);

                    ImGui::Spacing();
                    ImGui::Text("Target Drive Space:");
                    
                    ImVec4 color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green
                    const char* statusText = "Disk space is healthy.";

                    if (m_sourceSize > available) {
                        color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red
                        statusText = "CRITICAL: Not enough space for source files!";
                    } else if (requiredRatio > 0.8f || usedRatio > 0.9f) {
                        color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
                        statusText = "Warning: Space will be very tight after processing.";
                    } else if (usedRatio > 0.75f) {
                        color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Yellow
                        statusText = "Note: Target drive is more than 75% full.";
                    }

                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "%.1f GB free / %.1f GB total (Source needs %.1f GB)", 
                        static_cast<double>(available) / (1024*1024*1024), 
                        static_cast<double>(capacity) / (1024*1024*1024),
                        static_cast<double>(m_sourceSize) / (1024*1024*1024));
                    ImGui::ProgressBar(usedRatio, ImVec2(-1, 20), buf);
                    ImGui::PopStyleColor();
                    ImGui::TextColored(color, "%s", statusText);
                } catch (...) {
                    // Ignore errors
                }
            }
        }

        if (ImGui::CollapsingHeader("Organization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Folder Structure Pattern:");
            char patternBuf[256];
            std::strncpy(patternBuf, m_editedSettings.folderPattern.c_str(), sizeof(patternBuf));
            if (ImGui::InputText("##FolderPattern", patternBuf, sizeof(patternBuf))) {
                m_editedSettings.folderPattern = patternBuf;
                m_isDirty = true;
            }
            ImGui::SetItemTooltip("Use standard strftime format codes.\n%%Y = Year (2024)\n%%m = Month (01-12)\n%%d = Day (01-31)\n%%B = Month Name (January)");

            ImGui::Text("Presets:");
            ImGui::SameLine();
            if (ImGui::Button("Year/Month/Day")) {
                m_editedSettings.folderPattern = "%Y/%m/%d";
                m_isDirty = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Year/Month")) {
                m_editedSettings.folderPattern = "%Y/%m";
                m_isDirty = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Year/Full Date")) {
                m_editedSettings.folderPattern = "%Y/%Y-%m-%d";
                m_isDirty = true;
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
                ImGui::SetItemTooltip("Copy: Keep original files.\nMove: Transfer files to new location (deletes originals).");
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
                ImGui::SetItemTooltip("None: No check.\nSizeOnly: Check file size.\nPartial: Check first 1MB hash.\nFull: Check entire file hash.");
                ImGui::PopItemWidth();

                // Duplicate Action
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Duplicate Handling:");
                ImGui::TableSetColumnIndex(1);
                int dupAction = static_cast<int>(m_editedSettings.duplicateAction);
                const char* dupActions[] = { "Skip (Safest)", "Overwrite (Dangerous)", "Rename (e.g. file (1).jpg)" };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##DupAction", &dupAction, dupActions, 3)) {
                    m_editedSettings.duplicateAction = static_cast<engine::DuplicateAction>(dupAction);
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("What to do if a file already exists in the target directory.");
                ImGui::PopItemWidth();

                ImGui::EndTable();
            }

            ImGui::Spacing();
            if (ImGui::Checkbox("Ask for each duplicate", &m_editedSettings.askOnDuplicate)) m_isDirty = true;
            ImGui::SetItemTooltip("Show a dialog for every duplicate encountered to choose manually.");

            if (ImGui::Checkbox("Dry Run (Simulation Mode)", &m_editedSettings.dryRun)) m_isDirty = true;
            ImGui::SetItemTooltip("Simulate the process without actually moving or copying any files.");
            
            if (ImGui::Checkbox("Auto-Start on selection", &m_editedSettings.autoStart)) m_isDirty = true;
            ImGui::SetItemTooltip("Automatically start the sorting process when a valid source and target are selected.");
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("SAVE ALL SETTINGS", ImVec2(150, 40))) {
            config.setSettings(m_editedSettings);
            config.save();
            m_isDirty = false;
        }
        ImGui::SetItemTooltip("Apply and persist these settings to disk.");

        ImGui::SameLine();
        if (ImGui::Button("DISCARD CHANGES", ImVec2(150, 40))) {
            m_editedSettings = config.getSettings();
            m_isDirty = false;
        }
        ImGui::SetItemTooltip("Discard changes and reload last saved settings.");
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
