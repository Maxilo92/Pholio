#pragma once

#include <string>
#include <future>
#include <atomic>
#include <vector>
#include <mutex>

namespace core {

struct UpdateInfo {
    bool hasUpdate = false;
    std::string latestVersion;
    std::string releaseUrl;
    std::string releaseNotes;
};

struct ApiLogEntry {
    std::string timestamp;
    std::string endpoint;
    int statusCode; // 0 if failed
    std::string response;
};

class UpdateManager {
public:
    static UpdateManager& getInstance();

    void checkForUpdates();
    bool isChecking() const { return m_isChecking; }
    const UpdateInfo& getUpdateInfo() const { return m_updateInfo; }
    void reset() { m_updateInfo = UpdateInfo{}; }
    
    // API Logging
    std::vector<ApiLogEntry> getApiLogs();
    void clearLogs();

private:
    UpdateManager() = default;
    ~UpdateManager() = default;

    void addLog(const std::string& endpoint, int status, const std::string& response);

    std::atomic<bool> m_isChecking{false};
    UpdateInfo m_updateInfo;
    std::future<void> m_updateFuture;
    
    std::mutex m_logMutex;
    std::vector<ApiLogEntry> m_apiLogs;
};

} // namespace core
