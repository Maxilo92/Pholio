#include "StructureAnalyzer.hpp"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace engine {

StructureAnalyzer::StructureAnalyzer(std::filesystem::path baseDestPath)
    : m_baseDestPath(std::move(baseDestPath)) {}

std::filesystem::path StructureAnalyzer::generatePath(const MediaMetadata& metadata) const {
    auto time = std::chrono::system_clock::to_time_t(metadata.creationTime);
    
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    std::stringstream ss;
    if (tm_buf.tm_year > 0) { // Simple check for valid-ish time
        // Pattern: YYYY/MM/DD
        ss << std::put_time(&tm_buf, "%Y/%m/%d");
    } else {
        ss << "Unknown";
    }

    std::filesystem::path targetDir = m_baseDestPath / ss.str();
    return targetDir / metadata.path.filename();
}

} // namespace engine
