#include "UpdateManager.hpp"
#include "core/Config.hpp"
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace core {

UpdateManager& UpdateManager::getInstance() {
    static UpdateManager instance;
    return instance;
}

static std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static std::string getTimestamp() {
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

void UpdateManager::checkForUpdates() {
    if (m_isChecking) return;

    m_isChecking = true;
    m_updateFuture = std::async(std::launch::async, [this]() {
        const std::string url = "https://api.github.com/repos/Maxilo92/Pholio/releases/latest";
        try {
            // Build curl command with User-Agent and include headers to get status code
            std::string cmd = "curl -s -w \"\\n%{http_code}\" -H \"User-Agent: Pholio-App\" " + url;
            
            std::string output = exec(cmd.c_str());
            if (output.empty()) {
                addLog(url, 0, "No response from curl");
                m_isChecking = false;
                return;
            }

            // Split response and status code (last line)
            size_t lastNewline = output.find_last_of('\n');
            std::string response = output.substr(0, lastNewline);
            std::string statusStr = output.substr(lastNewline + 1);
            int status = 0;
            try { status = std::stoi(statusStr); } catch (...) {}

            addLog(url, status, response.substr(0, 500) + (response.length() > 500 ? "..." : ""));

            if (status != 200) {
                m_isChecking = false;
                return;
            }

            auto j = nlohmann::json::parse(response);
            if (j.contains("tag_name")) {
                std::string latestTag = j["tag_name"];
                std::string latestVersion = latestTag;
                if (!latestVersion.empty() && latestVersion[0] == 'v') {
                    latestVersion = latestVersion.substr(1);
                }

                std::string currentVersion = std::string(PROJECT_VERSION);
                
                // Better version comparison would be needed for true semver, 
                // but this works for sequential alpha/beta tags.
                if (latestVersion != currentVersion) {
                    m_updateInfo.hasUpdate = true;
                    m_updateInfo.latestVersion = latestVersion;
                    m_updateInfo.releaseUrl = j.value("html_url", "");
                    m_updateInfo.releaseNotes = j.value("body", "");
                }
            }
        } catch (const std::exception& e) {
            addLog(url, 0, std::string("Error: ") + e.what());
        }
        m_isChecking = false;
    });
}

std::vector<ApiLogEntry> UpdateManager::getApiLogs() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_apiLogs;
}

void UpdateManager::clearLogs() {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_apiLogs.clear();
}

void UpdateManager::addLog(const std::string& endpoint, int status, const std::string& response) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_apiLogs.push_back({getTimestamp(), endpoint, status, response});
    if (m_apiLogs.size() > 50) {
        m_apiLogs.erase(m_apiLogs.begin());
    }
}

} // namespace core
