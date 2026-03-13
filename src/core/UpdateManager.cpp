#include "UpdateManager.hpp"
#include "core/Config.hpp"
#include "core/ConfigManager.hpp"
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cctype>
#include <cstdlib>
#include <nlohmann/json.hpp>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

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

static std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
}

static bool endsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string toLower(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

static int scoreAssetName(const std::string& name) {
    const std::string lower = toLower(name);
    if (!endsWith(lower, ".zip")) return -1;

    int score = 10;
#if defined(__APPLE__)
    if (lower.find("mac") != std::string::npos || lower.find("darwin") != std::string::npos || lower.find("osx") != std::string::npos) {
        score += 50;
    }
    if (lower.find("arm64") != std::string::npos || lower.find("aarch64") != std::string::npos) {
        score += 10;
    }
#endif
    return score;
}

#if defined(__APPLE__)
static std::filesystem::path resolveExecutablePath() {
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0) return {};

    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) return {};

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(std::filesystem::path(buffer.c_str()), ec);
    if (!ec) return canonical;
    return std::filesystem::path(buffer.c_str());
}

static std::filesystem::path findAppBundlePath(const std::filesystem::path& execPath) {
    if (execPath.empty()) return {};
    auto p = execPath;
    for (int i = 0; i < 4 && !p.empty(); ++i) {
        if (p.extension() == ".app") return p;
        p = p.parent_path();
    }
    return {};
}
#endif

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
                    UpdateInfo nextInfo;
                    nextInfo.hasUpdate = true;
                    nextInfo.latestVersion = latestVersion;
                    nextInfo.releaseUrl = j.value("html_url", "");
                    nextInfo.releaseNotes = j.value("body", "");

                    if (j.contains("assets") && j["assets"].is_array()) {
                        int bestScore = -1;
                        for (const auto& asset : j["assets"]) {
                            if (!asset.contains("name") || !asset.contains("browser_download_url")) continue;
                            const std::string assetName = asset.value("name", "");
                            const std::string assetUrl = asset.value("browser_download_url", "");
                            const int score = scoreAssetName(assetName);
                            if (score > bestScore && !assetUrl.empty()) {
                                bestScore = score;
                                nextInfo.installAssetName = assetName;
                                nextInfo.installAssetUrl = assetUrl;
                            }
                        }
                    }
                    nextInfo.canInstallDirectly = !nextInfo.installAssetUrl.empty();

                    std::lock_guard<std::mutex> lock(m_updateMutex);
                    m_updateInfo = std::move(nextInfo);
                }
            }
        } catch (const std::exception& e) {
            addLog(url, 0, std::string("Error: ") + e.what());
        }
        m_isChecking = false;
    });
}

UpdateInfo UpdateManager::getUpdateInfo() const {
    std::lock_guard<std::mutex> lock(m_updateMutex);
    return m_updateInfo;
}

void UpdateManager::reset() {
    std::lock_guard<std::mutex> lock(m_updateMutex);
    m_updateInfo = UpdateInfo{};
}

bool UpdateManager::queueUpdateForNextRestart() {
    const auto info = getUpdateInfo();
    if (!info.hasUpdate || !info.canInstallDirectly || info.installAssetUrl.empty()) {
        return false;
    }

    auto& config = ConfigManager::getInstance();
    auto settings = config.getSettings();
    settings.pendingUpdateVersion = info.latestVersion;
    settings.pendingUpdateAssetUrl = info.installAssetUrl;
    settings.pendingUpdateAssetName = info.installAssetName;
    config.setSettings(settings);
    config.save();
    return true;
}

bool UpdateManager::installQueuedUpdateNow() {
    if (!queueUpdateForNextRestart()) {
        return false;
    }
    return applyPendingUpdateIfRequested();
}

std::string UpdateManager::getLastInstallError() const {
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastInstallError;
}

void UpdateManager::clearPendingUpdateSettings() {
    auto& config = ConfigManager::getInstance();
    auto settings = config.getSettings();
    settings.pendingUpdateVersion.clear();
    settings.pendingUpdateAssetUrl.clear();
    settings.pendingUpdateAssetName.clear();
    config.setSettings(settings);
    config.save();
}

bool UpdateManager::applyPendingUpdateIfRequested() {
    auto settings = ConfigManager::getInstance().getSettings();
    if (settings.pendingUpdateAssetUrl.empty() || settings.pendingUpdateVersion.empty()) {
        return false;
    }

#if defined(__APPLE__)
    const bool ok = applyPendingUpdateMacOS(settings.pendingUpdateVersion, settings.pendingUpdateAssetUrl, settings.pendingUpdateAssetName);
    if (ok) {
        clearPendingUpdateSettings();
        return true;
    }
    clearPendingUpdateSettings();
    return false;
#else
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Direct update install is currently supported on macOS only.";
    }
    addLog("pending-update", 0, m_lastInstallError);
    clearPendingUpdateSettings();
    return false;
#endif
}

#if defined(__APPLE__)
bool UpdateManager::applyPendingUpdateMacOS(const std::string& version, const std::string& assetUrl, const std::string& assetName) {
    (void)version;
    if (assetUrl.rfind("https://", 0) != 0) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Refusing update download: asset URL must use HTTPS.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const auto execPath = resolveExecutablePath();
    const auto appBundlePath = findAppBundlePath(execPath);
    if (appBundlePath.empty()) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: current app bundle path could not be resolved.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    std::error_code ec;
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    const auto workDir = std::filesystem::temp_directory_path(ec) /
        ("pholio-update-" + std::to_string(static_cast<long long>(::getpid())) + "-" + std::to_string(static_cast<long long>(now)));
    if (ec) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: unable to access temp directory.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    std::filesystem::create_directories(workDir, ec);
    if (ec) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: unable to create temp working directory.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const auto zipPath = workDir / (assetName.empty() ? "update.zip" : assetName);
    const std::string downloadCmd =
        "curl --fail --location --silent --show-error --proto '=https' --tlsv1.2 -o " +
        shellQuote(zipPath.string()) + " " + shellQuote(assetUrl);
    if (std::system(downloadCmd.c_str()) != 0) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update download failed.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    if (!std::filesystem::exists(zipPath, ec) || std::filesystem::file_size(zipPath, ec) == 0 || ec) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Downloaded update archive is missing or empty.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const auto extractedDir = workDir / "extracted";
    std::filesystem::create_directories(extractedDir, ec);
    if (ec) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: unable to prepare extraction directory.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const std::string unzipCmd = "unzip -q " + shellQuote(zipPath.string()) + " -d " + shellQuote(extractedDir.string());
    if (std::system(unzipCmd.c_str()) != 0) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: archive extraction error.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    std::filesystem::path extractedApp;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(extractedDir, ec)) {
        if (ec) break;
        if (entry.is_directory() && entry.path().extension() == ".app") {
            extractedApp = entry.path();
            break;
        }
    }
    if (extractedApp.empty()) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: extracted archive does not contain an .app bundle.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const auto backupPath = appBundlePath.parent_path() / (appBundlePath.filename().string() + ".backup");
    const auto scriptPath = workDir / "apply_update.sh";

    std::ofstream script(scriptPath);
    if (!script.is_open()) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: cannot create installer script.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const std::string currentAppQuoted = shellQuote(appBundlePath.string());
    const std::string backupQuoted = shellQuote(backupPath.string());
    const std::string newAppQuoted = shellQuote(extractedApp.string());
    const std::string workDirQuoted = shellQuote(workDir.string());

    script << "#!/bin/bash\n";
    script << "set -u\n";
    script << "TARGET_APP=" << currentAppQuoted << "\n";
    script << "BACKUP_APP=" << backupQuoted << "\n";
    script << "NEW_APP=" << newAppQuoted << "\n";
    script << "WORK_DIR=" << workDirQuoted << "\n";
    script << "OLD_PID=" << static_cast<long long>(::getpid()) << "\n";
    script << "LOG_FILE=\"$HOME/.config/Pholio/update-installer.log\"\n";
    script << "mkdir -p \"$(dirname \"$LOG_FILE\")\"\n";
    script << "exec >>\"$LOG_FILE\" 2>&1\n";
    script << "echo \"--- $(date '+%Y-%m-%d %H:%M:%S') update installer start ---\"\n";
    script << "echo \"target=$TARGET_APP\"\n";
    script << "echo \"new=$NEW_APP\"\n";
    script << "launch_app() {\n";
    script << "  APP_PATH=\"$1\"\n";
    script << "  BIN_PATH=\"$APP_PATH/Contents/MacOS/Pholio\"\n";
    script << "  if [ ! -x \"$BIN_PATH\" ]; then\n";
    script << "    return 1\n";
    script << "  fi\n";
    script << "  \"$BIN_PATH\" >/dev/null 2>&1 &\n";
    script << "  echo \"$!\"\n";
    script << "  return 0\n";
    script << "}\n";
    script << "relaunch_app() {\n";
    script << "  if [ -d \"$TARGET_APP\" ]; then\n";
    script << "    launch_app \"$TARGET_APP\" >/dev/null || true\n";
    script << "    return\n";
    script << "  fi\n";
    script << "  if [ -d \"$BACKUP_APP\" ]; then\n";
    script << "    launch_app \"$BACKUP_APP\" >/dev/null || true\n";
    script << "  fi\n";
    script << "}\n";
    script << "fail() {\n";
    script << "  echo \"ERROR: $1\"\n";
    script << "  if [ -d \"$BACKUP_APP\" ] && [ ! -d \"$TARGET_APP\" ]; then\n";
    script << "    mv \"$BACKUP_APP\" \"$TARGET_APP\" || true\n";
    script << "  fi\n";
    script << "  relaunch_app\n";
    script << "  exit 1\n";
    script << "}\n";
    script << "for _ in $(seq 1 120); do\n";
    script << "  if ! kill -0 \"$OLD_PID\" 2>/dev/null; then break; fi\n";
    script << "  sleep 1\n";
    script << "done\n";
    script << "rm -rf \"$BACKUP_APP\" || true\n";
    script << "if ! mv \"$TARGET_APP\" \"$BACKUP_APP\"; then fail \"cannot move current app to backup\"; fi\n";
    script << "if ! mv \"$NEW_APP\" \"$TARGET_APP\"; then\n";
    script << "  mv \"$BACKUP_APP\" \"$TARGET_APP\" || true\n";
    script << "  fail \"cannot move new app into place\"\n";
    script << "fi\n";
    script << "NEW_PID=\"$(launch_app \"$TARGET_APP\")\" || fail \"cannot launch updated app binary\"\n";
    script << "echo \"launched updated app pid=$NEW_PID\"\n";
    script << "sleep 3\n";
    script << "if ! kill -0 \"$NEW_PID\" 2>/dev/null; then\n";
    script << "  echo \"Updated app exited too quickly, rolling back.\"\n";
    script << "  rm -rf \"$TARGET_APP\" || true\n";
    script << "  mv \"$BACKUP_APP\" \"$TARGET_APP\" || fail \"rollback failed after launch crash\"\n";
    script << "  ROLLBACK_PID=\"$(launch_app \"$TARGET_APP\")\" || fail \"cannot relaunch rolled-back app\"\n";
    script << "  echo \"rollback app launched pid=$ROLLBACK_PID\"\n";
    script << "  exit 1\n";
    script << "fi\n";
    script << "rm -rf \"$BACKUP_APP\" || true\n";
    script << "rm -rf \"$WORK_DIR\" || true\n";
    script << "echo \"Update installer finished successfully and app is alive.\"\n";
    script << "exit 0\n";
    script.close();

    std::filesystem::permissions(scriptPath,
                                 std::filesystem::perms::owner_exec |
                                 std::filesystem::perms::owner_read |
                                 std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace, ec);
    if (ec) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: cannot make installer script executable.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    const std::string launchCmd = "nohup " + shellQuote(scriptPath.string()) + " >/dev/null 2>&1 &";
    if (std::system(launchCmd.c_str()) != 0) {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastInstallError = "Update install failed: unable to launch installer.";
        addLog("pending-update", 0, m_lastInstallError);
        return false;
    }

    addLog("pending-update", 200, "Update installer launched successfully.");
    return true;
}
#endif

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
