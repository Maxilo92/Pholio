#include "ReportManager.hpp"
#include "ConfigManager.hpp"
#include "core/Config.hpp"
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

namespace core {
namespace {

bool copyDirectoryTree(const std::filesystem::path& source,
                       const std::filesystem::path& destination,
                       std::string& outError) {
    std::error_code ec;
    if (!std::filesystem::exists(source, ec)) {
        return true;
    }
    if (ec) {
        outError = "Could not inspect source path: " + source.string();
        return false;
    }

    std::filesystem::create_directories(destination, ec);
    if (ec) {
        outError = "Could not create destination directory: " + destination.string();
        return false;
    }

    std::filesystem::copy(source,
                          destination,
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::overwrite_existing,
                          ec);
    if (ec) {
        outError = "Could not copy diagnostics from " + source.string();
        return false;
    }
    return true;
}

} // namespace

bool ReportManager::saveReport(const Report& report) {
    auto reportsDir = ConfigManager::getInstance().getReportsDirectory();
    
    if (!std::filesystem::exists(reportsDir)) {
        std::filesystem::create_directories(reportsDir);
    }

    // Generate filename: report_YYYYMMDD_HHMMSS.json
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "report_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".json";
    
    std::filesystem::path reportPath = reportsDir / ss.str();

    try {
        nlohmann::json j = report;
        std::ofstream file(reportPath);
        file << j.dump(4);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ReportManager::exportDiagnosticsBundle(std::filesystem::path& outBundlePath, std::string& outError) {
    auto& config = ConfigManager::getInstance();
    const auto reportsDir = config.getReportsDirectory();
    std::error_code ec;
    std::filesystem::create_directories(reportsDir, ec);
    if (ec) {
        outError = "Could not create reports directory.";
        return false;
    }

    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream stamp;
    stamp << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    outBundlePath = reportsDir / ("diagnostics_" + stamp.str());
    std::filesystem::create_directories(outBundlePath, ec);
    if (ec) {
        outError = "Could not create diagnostics bundle directory.";
        return false;
    }

    const auto logDir = config.getLogDirectory();
    const auto crashDir = config.getCrashesDirectory();
    const auto configPath = config.getConfigPath();

    if (!copyDirectoryTree(logDir, outBundlePath / "logs", outError)) {
        return false;
    }
    if (!copyDirectoryTree(crashDir, outBundlePath / "crashes", outError)) {
        return false;
    }

    if (std::filesystem::exists(configPath, ec) && !ec) {
        std::filesystem::create_directories(outBundlePath / "config", ec);
        if (ec) {
            outError = "Could not create diagnostics config directory.";
            return false;
        }
        std::filesystem::copy_file(configPath, outBundlePath / "config" / "settings.json",
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            outError = "Could not copy settings file.";
            return false;
        }
    }

    nlohmann::json meta;
    meta["version"] = PROJECT_VERSION;
    meta["created_at"] = stamp.str();
    meta["logs_included"] = std::filesystem::exists(logDir);
    meta["crashes_included"] = std::filesystem::exists(crashDir);
    meta["settings_included"] = std::filesystem::exists(configPath);

    std::ofstream metaFile(outBundlePath / "diagnostics.json");
    metaFile << meta.dump(4);
    if (!metaFile.good()) {
        outError = "Could not write diagnostics metadata.";
        return false;
    }

    return true;
}

std::string ReportManager::reportTypeToString(ReportType type) {
    switch (type) {
        case ReportType::Bug: return "Bug";
        case ReportType::Feature: return "Feature";
        case ReportType::Feedback: return "Feedback";
    }
    return "Unknown";
}

} // namespace core
