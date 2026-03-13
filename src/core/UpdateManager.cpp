#include "UpdateManager.hpp"
#include "core/Config.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

namespace core {

UpdateManager& UpdateManager::getInstance() {
    static UpdateManager instance;
    return instance;
}

static std::string exec(const char* cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
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

static std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

// Stable default: updates are disabled unless explicitly enabled.
static bool updatesEnabled() {
    const char* env = std::getenv("PHOLIO_UPDATES");
    if (!env) return false;
    std::string v(env);
    for (char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

void UpdateManager::checkForUpdates() {
    if (!updatesEnabled()) {
        m_status = UpdateStatus::Idle;
        return;
    }
    if (m_status == UpdateStatus::Checking) return;

    m_status = UpdateStatus::Checking;
    m_updateFuture = std::async(std::launch::async, [this]() {
        const std::string url = "https://api.github.com/repos/Maxilo92/Pholio/releases";
        try {
            std::string cmd = "curl -s -w \"\\n%{http_code}\" -H \"User-Agent: Pholio-App\" " + url;
            std::string output = exec(cmd.c_str());
            if (output.empty()) {
                addLog(url, 0, "No response from curl");
                m_status = UpdateStatus::Idle;
                return;
            }

            size_t lastNewline = output.find_last_of('\n');
            std::string response = output.substr(0, lastNewline);
            std::string statusStr = output.substr(lastNewline + 1);
            int status = 0;
            try { status = std::stoi(statusStr); } catch (...) {}

            addLog(url, status, response.substr(0, 500));
            if (status != 200) {
                m_status = UpdateStatus::Idle;
                return;
            }

            auto j = nlohmann::json::parse(response);
            if (!j.is_array() || j.empty()) {
                m_status = UpdateStatus::Idle;
                return;
            }

            auto latestRelease = j[0];
            std::string latestTag = latestRelease.value("tag_name", "");
            std::string latestVersion = latestTag;
            if (!latestVersion.empty() && latestVersion[0] == 'v') latestVersion = latestVersion.substr(1);

            if (!latestVersion.empty() && latestVersion != std::string(PROJECT_VERSION)) {
                m_updateInfo = UpdateInfo{};
                m_updateInfo.hasUpdate = true;
                m_updateInfo.latestVersion = latestVersion;
                m_updateInfo.releaseUrl = latestRelease.value("html_url", "");
                m_updateInfo.releaseNotes = latestRelease.value("body", "");

                if (latestRelease.contains("assets") && latestRelease["assets"].is_array()) {
                    for (auto& asset : latestRelease["assets"]) {
                        std::string name = asset.value("name", "");
                        if (name.find(".zip") != std::string::npos) {
                            m_updateInfo.downloadUrl = asset.value("browser_download_url", "");
                            break;
                        }
                    }
                    if (m_updateInfo.downloadUrl.empty() && !latestRelease["assets"].empty()) {
                        m_updateInfo.downloadUrl = latestRelease["assets"][0].value("browser_download_url", "");
                    }
                }

                m_status = UpdateStatus::UpdateAvailable;
                return;
            }
        } catch (const std::exception& e) {
            addLog(url, 0, std::string("Error: ") + e.what());
        }
        m_status = UpdateStatus::Idle;
    });
}

std::vector<std::string> UpdateManager::fetchAvailableVersions() {
    if (!updatesEnabled()) return {};
    std::vector<std::string> versions;
    const std::string url = "https://api.github.com/repos/Maxilo92/Pholio/releases";
    try {
        std::string output = exec(("curl -s -H \"User-Agent: Pholio-App\" " + url).c_str());
        if (output.empty()) return versions;
        auto j = nlohmann::json::parse(output);
        if (j.is_array()) {
            for (auto& release : j) {
                std::string tag = release.value("tag_name", "");
                if (!tag.empty() && tag[0] == 'v') tag = tag.substr(1);
                if (!tag.empty()) versions.push_back(tag);
            }
        }
    } catch (...) {}
    return versions;
}

bool UpdateManager::selectVersion(const std::string& version) {
    if (!updatesEnabled()) return false;
    const std::string url = "https://api.github.com/repos/Maxilo92/Pholio/releases";
    try {
        std::string output = exec(("curl -s -H \"User-Agent: Pholio-App\" " + url).c_str());
        if (output.empty()) return false;
        auto j = nlohmann::json::parse(output);
        if (!j.is_array()) return false;

        for (auto& release : j) {
            std::string tag = release.value("tag_name", "");
            std::string ver = tag;
            if (!ver.empty() && ver[0] == 'v') ver = ver.substr(1);
            if (ver == version || tag == version) {
                m_updateInfo = UpdateInfo{};
                m_updateInfo.hasUpdate = true;
                m_updateInfo.latestVersion = ver;
                m_updateInfo.releaseUrl = release.value("html_url", "");
                m_updateInfo.releaseNotes = release.value("body", "");
                if (release.contains("assets") && release["assets"].is_array() && !release["assets"].empty()) {
                    m_updateInfo.downloadUrl = release["assets"][0].value("browser_download_url", "");
                }
                m_status = UpdateStatus::UpdateAvailable;
                return true;
            }
        }
    } catch (...) {}
    return false;
}

bool UpdateManager::waitUntilFinished(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    while (m_status == UpdateStatus::Checking || m_status == UpdateStatus::Downloading || m_status == UpdateStatus::Installing) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > timeoutMs) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (m_updateFuture.valid()) m_updateFuture.wait();
    return true;
}

void UpdateManager::downloadUpdate() {
    if (!updatesEnabled()) {
        m_status = UpdateStatus::Idle;
        return;
    }
    if (m_status != UpdateStatus::UpdateAvailable || m_updateInfo.downloadUrl.empty()) return;

    m_status = UpdateStatus::Downloading;
    m_downloadProgress = 0.0f;

    m_updateFuture = std::async(std::launch::async, [this]() {
        if (m_updateInfo.downloadUrl.find("example.com") != std::string::npos) {
            for (int i = 0; i <= 10; ++i) {
                m_downloadProgress = i / 10.0f;
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
            m_status = UpdateStatus::ReadyToInstall;
            return;
        }

        std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "pholio_update.zip";
        std::string cmd = "curl -L -f -s -o " + shellQuote(tempPath.string()) + " " + shellQuote(m_updateInfo.downloadUrl);
        int result = std::system(cmd.c_str());
        if (result == 0) {
            m_downloadProgress = 1.0f;
            m_status = UpdateStatus::ReadyToInstall;
            addLog(m_updateInfo.downloadUrl, 200, "Download finished successfully");
        } else {
            m_status = UpdateStatus::DownloadFailed;
            addLog(m_updateInfo.downloadUrl, result, "Download failed");
        }
    });
}

void UpdateManager::installUpdate(bool restartNow) {
    if (!updatesEnabled()) {
        m_status = UpdateStatus::Idle;
        return;
    }
    if (m_status != UpdateStatus::ReadyToInstall) return;

    m_status = UpdateStatus::Installing;

    std::filesystem::path tempZip = std::filesystem::temp_directory_path() / "pholio_update.zip";
    if (!std::filesystem::exists(tempZip)) {
        m_status = UpdateStatus::InstallationFailed;
        return;
    }

    if (!restartNow) {
        m_status = UpdateStatus::Idle;
        return;
    }

    std::string appPath = getAppBundlePath();
    std::string execPath = getExecutablePath();
    std::string pid = std::to_string(getpid());
    std::filesystem::path launcherLogPath = std::filesystem::temp_directory_path() / "pholio_apply_update_launcher.log";

    if (appPath.empty() || execPath.empty()) {
        m_status = UpdateStatus::InstallationFailed;
        return;
    }

    std::string launchCmd =
        "nohup " + shellQuote(execPath) +
        " --apply-update" +
        " --zip " + shellQuote(tempZip.string()) +
        " --target-app " + shellQuote(appPath) +
        " --wait-pid " + shellQuote(pid) +
        " --relaunch" +
        " > " + shellQuote(launcherLogPath.string()) +
        " 2>&1 < /dev/null &";

    int launchResult = std::system(launchCmd.c_str());
    if (launchResult != 0) {
        m_status = UpdateStatus::InstallationFailed;
        return;
    }

    std::exit(0);
}

void UpdateManager::reset() {
    m_updateInfo = UpdateInfo{};
    m_status = UpdateStatus::Idle;
    m_downloadProgress = 0.0f;
}

void UpdateManager::triggerMockUpdate(const std::string& version) {
    m_updateInfo.hasUpdate = true;
    m_updateInfo.latestVersion = version;
    m_updateInfo.releaseUrl = "https://github.com/Maxilo92/Pholio/releases";
    m_updateInfo.releaseNotes = "### Mock Release\n- This is a simulated update.";
    m_updateInfo.downloadUrl = "https://example.com/mock_update.zip";
    m_status = UpdateStatus::UpdateAvailable;
}

void UpdateManager::triggerMockDownloadFinished() {
    m_downloadProgress = 1.0f;
    m_status = UpdateStatus::ReadyToInstall;
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
    if (m_apiLogs.size() > 50) m_apiLogs.erase(m_apiLogs.begin());
}

std::string UpdateManager::getExecutablePath() {
    char path[1024];
    uint32_t size = sizeof(path);
#ifdef __APPLE__
    if (_NSGetExecutablePath(path, &size) == 0) return std::string(path);
#endif
    return "";
}

std::string UpdateManager::getAppBundlePath() {
    std::string execPath = getExecutablePath();
    if (execPath.empty()) return "";

    std::filesystem::path p(execPath);
    if (p.parent_path().filename() == "MacOS" && p.parent_path().parent_path().filename() == "Contents") {
        return p.parent_path().parent_path().parent_path().string();
    }
    return execPath;
}

} // namespace core
