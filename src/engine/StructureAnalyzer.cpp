#include "StructureAnalyzer.hpp"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>

namespace engine {

namespace {
std::string replaceAll(std::string value, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return value;
    }
    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

std::string sanitizeFileName(std::string name) {
    for (char& c : name) {
        const bool invalid = (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' ||
                              c == '|' || c == '?' || c == '*');
        if (invalid || static_cast<unsigned char>(c) < 32) {
            c = '_';
        }
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
        name.pop_back();
    }
    if (name.empty()) {
        return "unnamed";
    }
    return name;
}
} // namespace

StructureAnalyzer::StructureAnalyzer(std::filesystem::path baseDestPath, std::string folderPattern, std::string filenameTemplate)
    : m_baseDestPath(std::move(baseDestPath)),
      m_folderPattern(std::move(folderPattern)),
      m_filenameTemplate(std::move(filenameTemplate)) {}

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
        // Use the pattern from settings
        ss << std::put_time(&tm_buf, m_folderPattern.c_str());
    } else {
        ss << "Unknown";
    }

    char yearBuf[5] = {};
    char monthBuf[3] = {};
    char dayBuf[3] = {};
    char hourBuf[3] = {};
    char minuteBuf[3] = {};
    char secondBuf[3] = {};
    std::strftime(yearBuf, sizeof(yearBuf), "%Y", &tm_buf);
    std::strftime(monthBuf, sizeof(monthBuf), "%m", &tm_buf);
    std::strftime(dayBuf, sizeof(dayBuf), "%d", &tm_buf);
    std::strftime(hourBuf, sizeof(hourBuf), "%H", &tm_buf);
    std::strftime(minuteBuf, sizeof(minuteBuf), "%M", &tm_buf);
    std::strftime(secondBuf, sizeof(secondBuf), "%S", &tm_buf);

    const std::string stem = metadata.path.stem().string();
    const std::string fullName = metadata.path.filename().string();
    std::string ext = metadata.path.extension().string();
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }

    std::string resolvedName = m_filenameTemplate.empty() ? "{original_filename}" : m_filenameTemplate;
    resolvedName = replaceAll(resolvedName, "{original_filename}", fullName);
    resolvedName = replaceAll(resolvedName, "{original_name}", stem);
    resolvedName = replaceAll(resolvedName, "{ext}", ext);
    resolvedName = replaceAll(resolvedName, "{yyyy}", yearBuf);
    resolvedName = replaceAll(resolvedName, "{MM}", monthBuf);
    resolvedName = replaceAll(resolvedName, "{dd}", dayBuf);
    resolvedName = replaceAll(resolvedName, "{HH}", hourBuf);
    resolvedName = replaceAll(resolvedName, "{mm}", minuteBuf);
    resolvedName = replaceAll(resolvedName, "{ss}", secondBuf);
    resolvedName = sanitizeFileName(resolvedName);

    std::filesystem::path filenamePath(resolvedName);
    if (!filenamePath.has_extension() && metadata.path.has_extension()) {
        resolvedName += metadata.path.extension().string();
    }

    std::filesystem::path targetDir = m_baseDestPath / ss.str();
    return targetDir / resolvedName;
}

} // namespace engine
