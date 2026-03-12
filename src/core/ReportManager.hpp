#pragma once

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace core {

enum class ReportType {
    Bug,
    Feature,
    Feedback
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReportType, {
    {ReportType::Bug, "Bug"},
    {ReportType::Feature, "Feature"},
    {ReportType::Feedback, "Feedback"}
})

struct Report {
    ReportType type = ReportType::Bug;
    std::string title;
    std::string description;
    std::string email;
    int priority = 2; // 1: Low, 2: Medium, 3: High
    std::string timestamp;
    std::string appVersion;
    std::string systemInfo;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Report, type, title, description, email, priority, timestamp, appVersion, systemInfo)
};

class ReportManager {
public:
    static bool saveReport(const Report& report);
    
private:
    static std::string reportTypeToString(ReportType type);
};

} // namespace core
