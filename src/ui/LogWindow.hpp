#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <imgui.h>

namespace ui {

enum class LogLevel {
    Info,
    Warning,
    Error,
    Success
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class LogWindow {
public:
    LogWindow();
    ~LogWindow();
    
    void log(LogLevel level, const std::string& message);
    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warn(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }
    void success(const std::string& message) { log(LogLevel::Success, message); }

    void render();
    void clear();

    void setupFileLogging(const std::filesystem::path& logDir);
    void openLogFolder() const;

private:
    std::filesystem::path m_logDir;
    std::vector<LogEntry> m_logs;
    std::mutex m_mutex;
    bool m_autoScroll = true;
    bool m_filterInfo = true;
    bool m_filterWarning = true;
    bool m_filterError = true;
    bool m_filterSuccess = true;
    char m_filterBuffer[128] = "";
    std::ofstream m_fileStream;
    
    static std::string getTimestamp();
    static ImVec4 getLevelColor(LogLevel level);
};

} // namespace ui
