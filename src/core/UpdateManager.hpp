#pragma once

#include <string>
#include <future>
#include <atomic>

namespace core {

struct UpdateInfo {
    bool hasUpdate = false;
    std::string latestVersion;
    std::string releaseUrl;
    std::string releaseNotes;
};

class UpdateManager {
public:
    static UpdateManager& getInstance();

    void checkForUpdates();
    bool isChecking() const { return m_isChecking; }
    const UpdateInfo& getUpdateInfo() const { return m_updateInfo; }
    void reset() { m_updateInfo = UpdateInfo{}; }

private:
    UpdateManager() = default;
    ~UpdateManager() = default;

    std::atomic<bool> m_isChecking{false};
    UpdateInfo m_updateInfo;
    std::future<void> m_updateFuture;
};

} // namespace core
