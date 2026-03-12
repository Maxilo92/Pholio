#include "UpdateManager.hpp"
#include "core/Config.hpp"
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
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
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void UpdateManager::checkForUpdates() {
    if (m_isChecking) return;

    m_isChecking = true;
    m_updateFuture = std::async(std::launch::async, [this]() {
        try {
            // Using curl to fetch latest release info from GitHub API
            const char* url = "https://api.github.com/repos/Maxilo92/Pholio/releases/latest";
            
            // Build curl command with User-Agent (required by GitHub API)
            std::string cmd = "curl -s -H \"User-Agent: Pholio-App\" " + std::string(url);
            
            std::string response = exec(cmd.c_str());
            if (response.empty() || response.find("{") == std::string::npos) {
                m_isChecking = false;
                return;
            }

            auto j = nlohmann::json::parse(response);
            if (j.contains("tag_name")) {
                std::string latestTag = j["tag_name"];
                // Strip 'v' prefix if present
                std::string latestVersion = latestTag;
                if (!latestVersion.empty() && latestVersion[0] == 'v') {
                    latestVersion = latestVersion.substr(1);
                }

                std::string currentVersion = std::string(PROJECT_VERSION);
                // Simple string comparison for versions (works for typical alpha/beta/semantic versions)
                if (latestVersion != currentVersion && latestVersion > currentVersion) {
                    m_updateInfo.hasUpdate = true;
                    m_updateInfo.latestVersion = latestVersion;
                    m_updateInfo.releaseUrl = j.value("html_url", "");
                    m_updateInfo.releaseNotes = j.value("body", "");
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Update check failed: " << e.what() << std::endl;
        }
        m_isChecking = false;
    });
}

} // namespace core
