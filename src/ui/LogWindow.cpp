#include "LogWindow.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

namespace ui {

LogWindow::LogWindow() {
    m_logs.reserve(1000);
}

LogWindow::~LogWindow() {
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void LogWindow::setupFileLogging(const std::filesystem::path& logDir) {
    m_logDir = logDir;
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directories(logDir);
    }

    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << "log_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".txt";
    
    std::filesystem::path logFile = logDir / ss.str();
    m_fileStream.open(logFile, std::ios::out | std::ios::app);
    
    if (m_fileStream.is_open()) {
        m_fileStream << "--- Log Session Started: " << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << " ---" << std::endl;
        m_fileStream.flush();
    }
}

void LogWindow::openLogFolder() const {
    if (m_logDir.empty() || !std::filesystem::exists(m_logDir)) return;

#ifdef _WIN32
    std::string command = "explorer \"" + m_logDir.string() + "\"";
#elif __APPLE__
    std::string command = "open \"" + m_logDir.string() + "\"";
#else
    std::string command = "xdg-open \"" + m_logDir.string() + "\"";
#endif
    std::system(command.c_str());
}

void LogWindow::log(LogLevel level, const std::string& message) {
    std::string timestamp = getTimestamp();
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.push_back({level, timestamp, message});
        
        if (m_logs.size() > 2000) {
            m_logs.erase(m_logs.begin(), m_logs.begin() + 500);
        }
    }

    if (m_fileStream.is_open()) {
        const char* levelStr = "INFO";
        switch (level) {
            case LogLevel::Warning: levelStr = "WARN"; break;
            case LogLevel::Error:   levelStr = "ERROR"; break;
            case LogLevel::Success: levelStr = "SUCCESS"; break;
            default: break;
        }
        m_fileStream << "[" << timestamp << "] [" << levelStr << "] " << message << std::endl;
        m_fileStream.flush();
    }
}

void LogWindow::render() {
    if (!ImGui::Begin("Logs")) {
        ImGui::End();
        return;
    }

    // Top Row: Primary Actions
    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    if (ImGui::Button("Open Log Folder")) openLogFolder();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    if (ImGui::InputText("Search", m_filterBuffer, sizeof(m_filterBuffer))) {
        // Search happens in loop
    }
    ImGui::PopItemWidth();

    // Second Row: Filter Toggles
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_filterInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warn", &m_filterWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &m_filterError);
    ImGui::SameLine();
    ImGui::Checkbox("Success", &m_filterSuccess);

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& entry : m_logs) {
        // Apply level filters
        if (entry.level == LogLevel::Info && !m_filterInfo) continue;
        if (entry.level == LogLevel::Warning && !m_filterWarning) continue;
        if (entry.level == LogLevel::Error && !m_filterError) continue;
        if (entry.level == LogLevel::Success && !m_filterSuccess) continue;

        // Apply text filter
        if (m_filterBuffer[0] != '\0' && entry.message.find(m_filterBuffer) == std::string::npos)
            continue;

        ImGui::TextDisabled("[%s]", entry.timestamp.c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, getLevelColor(entry.level));
        ImGui::TextUnformatted(entry.message.c_str());
        ImGui::PopStyleColor();

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Copy Message")) {
                ImGui::SetClipboardText(entry.message.c_str());
            }
            if (ImGui::MenuItem("Clear Logs")) {
                clear();
            }
            ImGui::EndPopup();
        }
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void LogWindow::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
}

std::string LogWindow::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

ImVec4 LogWindow::getLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Info:    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        case LogLevel::Warning: return ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Yellow/Orange
        case LogLevel::Error:   return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red
        case LogLevel::Success: return ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green
        default:                return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

} // namespace ui
