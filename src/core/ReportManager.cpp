#include "ReportManager.hpp"
#include "ConfigManager.hpp"
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

namespace core {

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
    } catch (...) {
        return false;
    }
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
