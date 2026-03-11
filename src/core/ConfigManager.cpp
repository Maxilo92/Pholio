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
        m_settings = j.get<AppSettings>();
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
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
    return getConfigPath().parent_path() / "logs";
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
