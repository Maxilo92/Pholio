#pragma once

#include <filesystem>
#include <string>
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
}

namespace core {

struct AppSettings {
    std::filesystem::path sourcePath;
    std::filesystem::path targetPath;
    engine::OperationMode operationMode = engine::OperationMode::Copy;
    engine::VerificationLevel verificationLevel = engine::VerificationLevel::Full;
    bool dryRun = false;
    bool autoStart = false;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AppSettings, 
        sourcePath, 
        targetPath, 
        operationMode, 
        verificationLevel, 
        dryRun, 
        autoStart
    )
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

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    AppSettings m_settings;
    mutable std::mutex m_mutex;
    std::filesystem::path getConfigPath() const;
};

} // namespace core
