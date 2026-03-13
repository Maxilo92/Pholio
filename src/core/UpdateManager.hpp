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
    std::string installAssetUrl;
    std::string installAssetName;
    bool canInstallDirectly = false;
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
    UpdateInfo getUpdateInfo() const;
    void reset();
    bool queueUpdateForNextRestart();
    bool installQueuedUpdateNow();
    bool applyPendingUpdateIfRequested();
    std::string getLastInstallError() const;
    
    // API Logging
    std::vector<ApiLogEntry> getApiLogs();
    void clearLogs();

private:
    UpdateManager() = default;
    ~UpdateManager() = default;

    void addLog(const std::string& endpoint, int status, const std::string& response);
    void clearPendingUpdateSettings();
    bool applyPendingUpdateMacOS(const std::string& version, const std::string& assetUrl, const std::string& assetName);

    std::atomic<bool> m_isChecking{false};
    UpdateInfo m_updateInfo;
    std::future<void> m_updateFuture;
    mutable std::mutex m_updateMutex;
    mutable std::mutex m_errorMutex;
    std::string m_lastInstallError;
    
    std::mutex m_logMutex;
    std::vector<ApiLogEntry> m_apiLogs;
};

} // namespace core
