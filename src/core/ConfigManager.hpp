#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../engine/Sorter.hpp"
#include "../engine/Verifier.hpp"

namespace nlohmann {
    template <>
    struct adl_serializer<std::filesystem::path> {
        static void to_json(json& j, const std::filesystem::path& p) {
            j = p.string();
        }

        static void from_json(const json& j, std::filesystem::path& p) {
            p = j.get<std::string>();
        }
    };
}

namespace engine {
    NLOHMANN_JSON_SERIALIZE_ENUM(OperationMode, {
        {OperationMode::Copy, "Copy"},
        {OperationMode::Move, "Move"},
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(VerificationLevel, {
        {VerificationLevel::None, "None"},
        {VerificationLevel::SizeOnly, "SizeOnly"},
        {VerificationLevel::Partial, "Partial"},
        {VerificationLevel::Full, "Full"},
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(DuplicateAction, {
        {DuplicateAction::Skip, "Skip"},
        {DuplicateAction::Overwrite, "Overwrite"},
        {DuplicateAction::Rename, "Rename"},
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(MigrationMode, {
        {MigrationMode::Rebuild, "Rebuild"},
        {MigrationMode::Merge, "Merge"},
        {MigrationMode::ContinueExisting, "ContinueExisting"},
    })
}

namespace core {

struct AppSettings {
    std::filesystem::path sourcePath;
    std::filesystem::path targetPath;
    engine::OperationMode operationMode = engine::OperationMode::Copy;
    engine::VerificationLevel verificationLevel = engine::VerificationLevel::Full;
    engine::DuplicateAction duplicateAction = engine::DuplicateAction::Skip;
    bool askOnDuplicate = true;
    bool dryRun = false;
    bool showPreview = true;
    bool enableFormatConversion = false;
    std::string imageOutputFormat = "jpg";
    std::string videoOutputFormat = "mp4";
    bool enablePlugins = false;
    std::filesystem::path pluginsDirectory = "plugins";
    std::vector<std::string> disabledPlugins;
    bool allowPluginWindows = true;
    int pluginReloadToken = 0;
    engine::MigrationMode migrationMode = engine::MigrationMode::Merge;
    std::string lastVersion = "0.0.0";
    std::string folderPattern = "%Y/%m-%B/%d";
    std::string filenameTemplate = "{original_filename}";
    bool showDashboard = true;
    bool showSettings = false;
    bool showProgress = false;
    bool showLogs = true;
    bool showReport = false;
    bool showPlugins = false;
    std::string uiLanguage = "en";
    std::string uiTheme = "dark";
    std::string pendingUpdateVersion;
    std::string pendingUpdateAssetUrl;
    std::string pendingUpdateAssetName;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AppSettings, sourcePath, targetPath, operationMode, verificationLevel, duplicateAction, askOnDuplicate, dryRun, showPreview, enableFormatConversion, imageOutputFormat, videoOutputFormat, enablePlugins, pluginsDirectory, disabledPlugins, allowPluginWindows, pluginReloadToken, migrationMode, lastVersion, folderPattern, filenameTemplate, showDashboard, showSettings, showProgress, showLogs, showReport, showPlugins, uiLanguage, uiTheme, pendingUpdateVersion, pendingUpdateAssetUrl, pendingUpdateAssetName)
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    void load();
    void save();

    AppSettings getSettings() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_settings; 
    }
    
    void setSettings(const AppSettings& settings) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_settings = settings;
    }

    std::filesystem::path getLogDirectory() const;
    std::filesystem::path getCrashesDirectory() const;
    std::filesystem::path getReportsDirectory() const;
    std::filesystem::path getImguiConfigPath() const;
    std::filesystem::path getConfigPath() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    AppSettings m_settings;
    mutable std::mutex m_mutex;
};

} // namespace core
