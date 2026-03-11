#include "LogWindow.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

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
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directories(logDir);
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
    }
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

    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& entry : m_logs) {
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
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%H:%M:%S");
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
