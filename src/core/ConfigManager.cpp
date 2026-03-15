#include "ConfigManager.hpp"
#include "core/Config.hpp"
#include <fstream>
#include <iostream>
#include <mutex>

namespace core {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::load() {
    std::filesystem::path configPath = getConfigPath();
    if (!std::filesystem::exists(configPath)) {
        return;
    }

    try {
        std::ifstream file(configPath);
        nlohmann::json j;
        file >> j;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        // Use a safer way to load to avoid exceptions on missing keys
        if (j.contains("sourcePath")) m_settings.sourcePath = j.at("sourcePath").get<std::filesystem::path>();
        if (j.contains("targetPath")) m_settings.targetPath = j.at("targetPath").get<std::filesystem::path>();
        if (j.contains("operationMode")) m_settings.operationMode = j.at("operationMode").get<engine::OperationMode>();
        if (j.contains("verificationLevel")) m_settings.verificationLevel = j.at("verificationLevel").get<engine::VerificationLevel>();
        if (j.contains("duplicateAction")) m_settings.duplicateAction = j.at("duplicateAction").get<engine::DuplicateAction>();
        if (j.contains("askOnDuplicate")) m_settings.askOnDuplicate = j.at("askOnDuplicate").get<bool>();
        if (j.contains("dryRun")) m_settings.dryRun = j.at("dryRun").get<bool>();
        if (j.contains("showPreview")) m_settings.showPreview = j.at("showPreview").get<bool>();
        if (j.contains("showGallery")) m_settings.showGallery = j.at("showGallery").get<bool>();
        if (j.contains("enableFormatConversion")) m_settings.enableFormatConversion = j.at("enableFormatConversion").get<bool>();
        if (j.contains("imageOutputFormat")) m_settings.imageOutputFormat = j.at("imageOutputFormat").get<std::string>();
        if (j.contains("videoOutputFormat")) m_settings.videoOutputFormat = j.at("videoOutputFormat").get<std::string>();
        if (j.contains("enablePlugins")) m_settings.enablePlugins = j.at("enablePlugins").get<bool>();
        if (j.contains("pluginsDirectory")) m_settings.pluginsDirectory = j.at("pluginsDirectory").get<std::filesystem::path>();
        if (j.contains("disabledPlugins")) m_settings.disabledPlugins = j.at("disabledPlugins").get<std::vector<std::string>>();
        if (j.contains("allowPluginWindows")) m_settings.allowPluginWindows = j.at("allowPluginWindows").get<bool>();
        if (j.contains("pluginReloadToken")) m_settings.pluginReloadToken = j.at("pluginReloadToken").get<int>();
        if (j.contains("migrationMode")) m_settings.migrationMode = j.at("migrationMode").get<engine::MigrationMode>();
        if (j.contains("lastVersion")) m_settings.lastVersion = j.at("lastVersion").get<std::string>();
        if (j.contains("folderPattern")) m_settings.folderPattern = j.at("folderPattern").get<std::string>();
        if (j.contains("filenameTemplate")) m_settings.filenameTemplate = j.at("filenameTemplate").get<std::string>();
        if (j.contains("showDashboard")) m_settings.showDashboard = j.at("showDashboard").get<bool>();
        if (j.contains("showSettings")) m_settings.showSettings = j.at("showSettings").get<bool>();
        if (j.contains("showProgress")) m_settings.showProgress = j.at("showProgress").get<bool>();
        if (j.contains("showLogs")) m_settings.showLogs = j.at("showLogs").get<bool>();
        if (j.contains("showReport")) m_settings.showReport = j.at("showReport").get<bool>();
        if (j.contains("showPlugins")) m_settings.showPlugins = j.at("showPlugins").get<bool>();
        if (j.contains("uiLanguage")) m_settings.uiLanguage = j.at("uiLanguage").get<std::string>();
        if (j.contains("uiTheme")) m_settings.uiTheme = j.at("uiTheme").get<std::string>();
        if (j.contains("autoUpdateCheckIntervalSeconds")) m_settings.autoUpdateCheckIntervalSeconds = j.at("autoUpdateCheckIntervalSeconds").get<int>();
        if (j.contains("pendingUpdateVersion")) m_settings.pendingUpdateVersion = j.at("pendingUpdateVersion").get<std::string>();
        if (j.contains("pendingUpdateAssetUrl")) m_settings.pendingUpdateAssetUrl = j.at("pendingUpdateAssetUrl").get<std::string>();
        if (j.contains("pendingUpdateAssetName")) m_settings.pendingUpdateAssetName = j.at("pendingUpdateAssetName").get<std::string>();
        
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load some config values: " << e.what() << std::endl;
    }
}

void ConfigManager::save() {
    std::filesystem::path configPath = getConfigPath();
    std::filesystem::create_directories(configPath.parent_path());

    try {
        nlohmann::json j;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            j = m_settings;
        }
        std::ofstream file(configPath);
        file << j.dump(4);
    } catch (const std::exception& e) {
        std::cerr << "Failed to save config: " << e.what() << std::endl;
    }
}

std::filesystem::path ConfigManager::getLogDirectory() const {
    return std::filesystem::current_path() / "logs";
}

std::filesystem::path ConfigManager::getCrashesDirectory() const {
    return getConfigPath().parent_path() / "crashes";
}

std::filesystem::path ConfigManager::getReportsDirectory() const {
    return std::filesystem::current_path() / "reports";
}

std::filesystem::path ConfigManager::getImguiConfigPath() const {
    return getConfigPath().parent_path() / "imgui.ini";
}

std::filesystem::path ConfigManager::getConfigPath() const {
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    if (appData) {
        return std::filesystem::path(appData) / std::string(PROJECT_NAME) / "settings.json";
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".config" / std::string(PROJECT_NAME) / "settings.json";
    }
#endif
    return std::filesystem::current_path() / "settings.json";
}

} // namespace core
