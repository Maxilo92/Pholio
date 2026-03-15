#include "SettingsWindow.hpp"
#include <imgui.h>
#include <nfd.hpp>
#include <cstring>
#include <string>
#include <ctime>
#include <cstdlib>
#include "I18n.hpp"

namespace ui {

namespace {
std::string buildPatternExample(const std::string& pattern) {
    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif
    char out[512];
    const size_t written = std::strftime(out, sizeof(out), pattern.c_str(), &tmBuf);
    if (written == 0) return {};
    return std::string(out, written);
}
} // namespace

void SettingsWindow::render() {
    auto& config = core::ConfigManager::getInstance();
    auto tr = [](const char* key, const char* fallback) { return i18n::tr(key, fallback); };
    
    // Sync from global config if we aren't currently editing something unsaved
    if (!m_initialized || !m_isDirty) {
        m_editedSettings = config.getSettings();
        m_initialized = true;
    }

    if (ImGui::Begin("Settings")) {
        if (m_isDirty) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
            ImGui::TextWrapped("%s", tr("settings.unsaved", "You have unsaved changes!"));
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem(tr("settings.tab.directories", "Directories"))) {
                if (ImGui::BeginTable("DirectorySettings", 3, ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", tr("settings.source_path", "Source Path:"));
                    ImGui::TableSetColumnIndex(1);
                    char sourceBuf[1024];
                    std::strncpy(sourceBuf, m_editedSettings.sourcePath.string().c_str(), sizeof(sourceBuf));
                    ImGui::PushItemWidth(-FLT_MIN);
                    if (ImGui::InputText("##SourceSet", sourceBuf, sizeof(sourceBuf))) {
                        m_editedSettings.sourcePath = sourceBuf;
                        m_isDirty = true;
                    }
                    ImGui::SetItemTooltip("%s", tr("settings.tooltip.source", "The directory where your unorganized photos and videos are located."));
                    ImGui::PopItemWidth();
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button((std::string(tr("settings.browse", "Browse...")) + "##SourceSet").c_str())) m_shouldBrowseSource = true;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", tr("settings.target_path", "Target Path:"));
                    ImGui::TableSetColumnIndex(1);
                    char targetBuf[1024];
                    std::strncpy(targetBuf, m_editedSettings.targetPath.string().c_str(), sizeof(targetBuf));
                    ImGui::PushItemWidth(-FLT_MIN);
                    if (ImGui::InputText("##TargetSet", targetBuf, sizeof(targetBuf))) {
                        m_editedSettings.targetPath = targetBuf;
                        m_isDirty = true;
                    }
                    ImGui::SetItemTooltip("%s", tr("settings.tooltip.target", "The root directory where the organized library will be created."));
                    ImGui::PopItemWidth();
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button((std::string(tr("settings.browse", "Browse...")) + "##TargetSet").c_str())) m_shouldBrowseTarget = true;

                    ImGui::EndTable();
                }

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

                    ImGui::Text(tr("settings.source_size", "Source Size: %.2f GB %s"),
                                static_cast<double>(m_sourceSize) / (1024.0 * 1024.0 * 1024.0),
                                m_isCalculatingSourceSize ? tr("settings.calculating", "(calculating...)") : "");
                }

                if (!m_editedSettings.targetPath.empty() && std::filesystem::exists(m_editedSettings.targetPath)) {
                    try {
                        auto space = std::filesystem::space(m_editedSettings.targetPath);
                        uint64_t capacity = space.capacity;
                        uint64_t available = space.available;
                        uint64_t used = capacity - available;

                        float usedRatio = static_cast<float>(used) / static_cast<float>(capacity);
                        float requiredRatio = static_cast<float>(m_sourceSize) / static_cast<float>(available);

                        ImGui::Spacing();
                        ImGui::Text("%s", tr("settings.target_drive_space", "Target Drive Space:"));

                        ImVec4 color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
                        const char* statusText = tr("settings.disk_ok", "Disk space is healthy.");

                        if (m_sourceSize > available) {
                            color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                            statusText = tr("settings.disk_critical", "CRITICAL: Not enough space for source files!");
                        } else if (requiredRatio > 0.8f || usedRatio > 0.9f) {
                            color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
                            statusText = tr("settings.disk_warning", "Warning: Space will be very tight after processing.");
                        } else if (usedRatio > 0.75f) {
                            color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
                            statusText = tr("settings.disk_note", "Note: Target drive is more than 75% full.");
                        }

                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
                        char buf[128];
                        std::snprintf(buf, sizeof(buf), tr("settings.disk_progress", "%.1f GB free / %.1f GB total (Source needs %.1f GB)"),
                                      static_cast<double>(available) / (1024 * 1024 * 1024),
                                      static_cast<double>(capacity) / (1024 * 1024 * 1024),
                                      static_cast<double>(m_sourceSize) / (1024 * 1024 * 1024));
                        ImGui::ProgressBar(usedRatio, ImVec2(-1, 20), buf);
                        ImGui::PopStyleColor();
                        ImGui::PushStyleColor(ImGuiCol_Text, color);
                        ImGui::TextWrapped("%s", statusText);
                        ImGui::PopStyleColor();
                    } catch (...) {
                    }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(tr("settings.tab.folder_structure", "Folder Structure"))) {
                ImGui::Text("%s", tr("settings.folder_pattern", "Folder Structure Pattern:"));
                char patternBuf[256];
                std::strncpy(patternBuf, m_editedSettings.folderPattern.c_str(), sizeof(patternBuf));
                if (ImGui::InputText("##FolderPattern", patternBuf, sizeof(patternBuf))) {
                    m_editedSettings.folderPattern = patternBuf;
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.pattern", "Use strftime codes.\n%%Y=Year  %%m=Month  %%d=Day  %%B=Month Name"));

                ImGui::Spacing();
                ImGui::Text("%s", tr("settings.migration_strategy", "Migration Strategy:"));
                int migrationMode = static_cast<int>(m_editedSettings.migrationMode);
                const char* migrationModes[] = {
                    tr("settings.migration.rebuild", "Umbau (bestehende Struktur in neues Muster migrieren)"),
                    tr("settings.migration.merge", "Merge (erkannte Zielstruktur weiterverwenden)"),
                    tr("settings.migration.continue", "Weiterfuehren (erkannte Zielstruktur uebernehmen)")
                };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##MigrationMode", &migrationMode, migrationModes, 3)) {
                    m_editedSettings.migrationMode = static_cast<engine::MigrationMode>(migrationMode);
                    m_isDirty = true;
                }
                ImGui::PopItemWidth();
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.migration",
                    "Umbau: Bestehende Dateien im Ziel werden ins konfigurierte Muster verschoben.\n"
                    "Merge: Neue Dateien nutzen die erkannte Zielstruktur, falls vorhanden.\n"
                    "Weiterfuehren: Wie Merge, uebernimmt erkannte Struktur zusaetzlich als neues Standardmuster."));

                const std::string example = buildPatternExample(m_editedSettings.folderPattern);
                ImGui::Spacing();
                if (!example.empty()) {
                    ImGui::Text(tr("settings.example.today", "Example (today): %s"), example.c_str());
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "%s", tr("settings.example.invalid", "Example (today): Invalid pattern"));
                }

                ImGui::Spacing();
                ImGui::Text("%s", tr("settings.presets", "Presets:"));
                if (ImGui::Button(tr("settings.preset.ymd", "Year/Month/Day"))) {
                    m_editedSettings.folderPattern = "%Y/%m/%d";
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("settings.preset.ym", "Year/Month"))) {
                    m_editedSettings.folderPattern = "%Y/%m";
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("settings.preset.yfull", "Year/Full Date"))) {
                    m_editedSettings.folderPattern = "%Y/%Y-%m-%d";
                    m_isDirty = true;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("%s", tr("settings.filename_template", "Filename Template:"));
                char filenameTemplateBuf[256];
                std::strncpy(filenameTemplateBuf, m_editedSettings.filenameTemplate.c_str(), sizeof(filenameTemplateBuf));
                if (ImGui::InputText("##FilenameTemplate", filenameTemplateBuf, sizeof(filenameTemplateBuf))) {
                    m_editedSettings.filenameTemplate = filenameTemplateBuf;
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.filename_template", "Available: {original_filename}, {original_name}, {ext}, {yyyy}, {MM}, {dd}, {HH}, {mm}, {ss}"));

                if (ImGui::Button(tr("settings.preset.original", "Use Original Filename"))) {
                    m_editedSettings.filenameTemplate = "{original_filename}";
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("settings.preset.date_name", "Date + Original Name"))) {
                    m_editedSettings.filenameTemplate = "{yyyy}{MM}{dd}_{original_name}.{ext}";
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("settings.preset.timestamp", "Timestamp"))) {
                    m_editedSettings.filenameTemplate = "{yyyy}{MM}{dd}_{HH}{mm}{ss}.{ext}";
                    m_isDirty = true;
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(tr("settings.tab.engine", "Engine"))) {
                ImGui::Text("%s", tr("settings.language", "Language"));
                int languageIndex = i18n::getLanguage() == i18n::Language::German ? 1 : 0;
                const char* languageItems[] = {
                    tr("settings.language.english", "English"),
                    tr("settings.language.german", "German")
                };
                if (ImGui::Combo("##UiLanguage", &languageIndex, languageItems, 2)) {
                    const auto selected = languageIndex == 1 ? i18n::Language::German : i18n::Language::English;
                    i18n::setLanguage(selected);
                    m_editedSettings.uiLanguage = i18n::languageCode(selected);
                    m_isDirty = true;
                }
                ImGui::Text("%s", tr("settings.theme", "Theme"));
                int themeIndex = m_editedSettings.uiTheme == "light" ? 1 : 0;
                const char* themeItems[] = {
                    tr("settings.theme.dark", "Dark"),
                    tr("settings.theme.light", "Light")
                };
                if (ImGui::Combo("##UiTheme", &themeIndex, themeItems, 2)) {
                    m_editedSettings.uiTheme = themeIndex == 1 ? "light" : "dark";
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.theme.tooltip", "Choose between dark and light UI themes."));
                ImGui::Separator();

                if (ImGui::BeginTable("EngineSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

                // Op Mode
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", tr("settings.operation_mode", "Operation Mode:"));
                ImGui::TableSetColumnIndex(1);
                int opMode = static_cast<int>(m_editedSettings.operationMode);
                const char* opModes[] = { tr("settings.op.copy", "Copy (Safe)"), tr("settings.op.move", "Move (Efficient)") };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##OpMode", &opMode, opModes, 2)) {
                    m_editedSettings.operationMode = static_cast<engine::OperationMode>(opMode);
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.opmode", "Copy: Keep original files.\nMove: Transfer files to new location (deletes originals)."));
                ImGui::PopItemWidth();

                // Verif Level
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", tr("settings.verification", "Verification:"));
                ImGui::TableSetColumnIndex(1);
                int verLevel = static_cast<int>(m_editedSettings.verificationLevel);
                const char* verLevels[] = {
                    tr("settings.ver.none", "None (Fastest)"),
                    tr("settings.ver.size", "Size Only"),
                    tr("settings.ver.partial", "Partial Hash"),
                    tr("settings.ver.full", "Full Hash (Safest)")
                };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##VerLevel", &verLevel, verLevels, 4)) {
                    m_editedSettings.verificationLevel = static_cast<engine::VerificationLevel>(verLevel);
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.verification", "None: No check.\nSizeOnly: Check file size.\nPartial: Check first 1MB hash.\nFull: Check entire file hash."));
                ImGui::PopItemWidth();

                // Duplicate Action
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", tr("settings.duplicate", "Duplicate Handling:"));
                ImGui::TableSetColumnIndex(1);
                int dupAction = static_cast<int>(m_editedSettings.duplicateAction);
                const char* dupActions[] = {
                    tr("settings.dup.skip", "Skip (Safest)"),
                    tr("settings.dup.overwrite", "Overwrite (Dangerous)"),
                    tr("settings.dup.rename", "Rename (e.g. file (1).jpg)")
                };
                ImGui::PushItemWidth(-FLT_MIN);
                if (ImGui::Combo("##DupAction", &dupAction, dupActions, 3)) {
                    m_editedSettings.duplicateAction = static_cast<engine::DuplicateAction>(dupAction);
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.duplicate", "What to do if a file already exists in the target directory."));
                ImGui::PopItemWidth();

                ImGui::EndTable();
            }

                ImGui::Spacing();
                if (ImGui::Checkbox(tr("settings.ask_duplicate", "Ask for each duplicate"), &m_editedSettings.askOnDuplicate)) m_isDirty = true;
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.ask_duplicate", "Show a dialog for every duplicate encountered to choose manually."));

                if (ImGui::Checkbox(tr("settings.dry_run", "Dry Run (Simulation Mode)"), &m_editedSettings.dryRun)) m_isDirty = true;
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.dry_run", "Simulate the process without actually moving or copying any files."));

                int updateIntervalSeconds = m_editedSettings.autoUpdateCheckIntervalSeconds;
                if (ImGui::InputInt(tr("settings.update_check_interval", "Update Check Interval (seconds)"), &updateIntervalSeconds, 5, 30)) {
                    if (updateIntervalSeconds < 0) updateIntervalSeconds = 0;
                    if (updateIntervalSeconds > 86400) updateIntervalSeconds = 86400;
                    m_editedSettings.autoUpdateCheckIntervalSeconds = updateIntervalSeconds;
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.update_check_interval", "How often Pholio checks for updates in the background. 0 disables automatic checks."));

                ImGui::Spacing();
                if (ImGui::Checkbox(tr("settings.enable_conversion", "Enable Format Conversion"), &m_editedSettings.enableFormatConversion)) m_isDirty = true;
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.enable_conversion", "Convert copied media into configured output formats after verified transfer."));

                ImGui::BeginDisabled(!m_editedSettings.enableFormatConversion);
                const char* imageFormats[] = {"jpg", "png", "webp", "heic", "tiff"};
                int imageIdx = 0;
                for (int i = 0; i < 5; ++i) {
                    if (m_editedSettings.imageOutputFormat == imageFormats[i]) {
                        imageIdx = i;
                        break;
                    }
                }
                if (ImGui::Combo(tr("settings.image_output", "Image Output Format"), &imageIdx, imageFormats, 5)) {
                    m_editedSettings.imageOutputFormat = imageFormats[imageIdx];
                    m_isDirty = true;
                }

                const char* videoFormats[] = {"mp4", "mov", "mkv"};
                int videoIdx = 0;
                for (int i = 0; i < 3; ++i) {
                    if (m_editedSettings.videoOutputFormat == videoFormats[i]) {
                        videoIdx = i;
                        break;
                    }
                }
                if (ImGui::Combo(tr("settings.video_output", "Video Output Format"), &videoIdx, videoFormats, 3)) {
                    m_editedSettings.videoOutputFormat = videoFormats[videoIdx];
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.video_output", "Video conversion uses FFmpeg with H.264/AAC defaults."));
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(tr("settings.tab.plugins", "Plugins"))) {
                if (ImGui::Checkbox(tr("settings.enable_plugins", "Enable Plugin System"), &m_editedSettings.enablePlugins)) m_isDirty = true;
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.enable_plugins", "Loads external plugins for filtering, target override, and optional plugin windows."));

                if (ImGui::Checkbox(tr("settings.allow_plugin_windows", "Allow Plugin Windows"), &m_editedSettings.allowPluginWindows)) m_isDirty = true;
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.allow_plugin_windows", "Allows plugins to render custom ImGui windows in the app."));

                char pluginDirBuf[1024];
                std::strncpy(pluginDirBuf, m_editedSettings.pluginsDirectory.string().c_str(), sizeof(pluginDirBuf));
                if (ImGui::InputText(tr("settings.plugin_directory", "Plugin Directory"), pluginDirBuf, sizeof(pluginDirBuf))) {
                    m_editedSettings.pluginsDirectory = pluginDirBuf;
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.plugin_directory", "Directory scanned for plugin libraries (.dylib/.so/.dll)."));

                if (ImGui::Button(tr("settings.create_plugin_dir", "Create Plugin Directory"))) {
                    try {
                        std::filesystem::create_directories(m_editedSettings.pluginsDirectory);
                    } catch (...) {}
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("settings.open_plugin_dir", "Open Plugin Directory"))) {
                    const std::string absolute = std::filesystem::absolute(m_editedSettings.pluginsDirectory).string();
#ifdef _WIN32
                    std::system(("start \"\" \"" + absolute + "\"").c_str());
#elif __APPLE__
                    std::system(("open \"" + absolute + "\"").c_str());
#else
                    std::system(("xdg-open \"" + absolute + "\"").c_str());
#endif
                }
                ImGui::SameLine();
                if (ImGui::Button(tr("settings.reload_plugins_now", "Reload Plugins Now"))) {
                    m_editedSettings.pluginReloadToken += 1;
                    m_isDirty = true;
                }
                ImGui::SetItemTooltip("%s", tr("settings.tooltip.reload_plugins_now", "Triggers immediate plugin reload after saving settings."));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextWrapped("%s", tr("settings.workflow", "User-friendly workflow:"));
                ImGui::BulletText("%s", tr("settings.workflow.1", "1) Set plugin directory"));
                ImGui::BulletText("%s", tr("settings.workflow.2", "2) Add plugin files in Plugins window"));
                ImGui::BulletText("%s", tr("settings.workflow.3", "3) Click Reload Plugins Now"));
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(tr("settings.save_all", "SAVE ALL SETTINGS"), ImVec2(150, 40))) {
            config.setSettings(m_editedSettings);
            config.save();
            m_isDirty = false;
        }
        ImGui::SetItemTooltip("%s", tr("settings.tooltip.save", "Apply and persist these settings to disk."));

        ImGui::SameLine();
        if (ImGui::Button(tr("settings.discard", "DISCARD CHANGES"), ImVec2(150, 40))) {
            m_editedSettings = config.getSettings();
            i18n::setLanguage(i18n::languageFromCode(m_editedSettings.uiLanguage));
            m_isDirty = false;
        }
        ImGui::SetItemTooltip("%s", tr("settings.tooltip.discard", "Discard changes and reload last saved settings."));
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
