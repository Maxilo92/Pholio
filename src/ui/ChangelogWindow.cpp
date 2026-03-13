#include "ChangelogWindow.hpp"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <tuple>

namespace ui {

namespace {
struct ParsedVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::string suffix;
    bool valid = false;
};

ParsedVersion parseVersion(const std::string& version) {
    std::regex semverRegex(R"(^\s*(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9.\-]+))?\s*$)");
    std::smatch m;
    if (!std::regex_match(version, m, semverRegex)) return {};

    ParsedVersion out;
    out.major = std::stoi(m[1].str());
    out.minor = std::stoi(m[2].str());
    out.patch = std::stoi(m[3].str());
    out.suffix = m[4].matched ? m[4].str() : "";
    out.valid = true;
    return out;
}

int compareVersions(const ParsedVersion& a, const ParsedVersion& b) {
    if (a.major != b.major) return a.major < b.major ? -1 : 1;
    if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
    if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
    if (a.suffix == b.suffix) return 0;
    if (a.suffix.empty()) return 1;
    if (b.suffix.empty()) return -1;
    return a.suffix < b.suffix ? -1 : (a.suffix > b.suffix ? 1 : 0);
}
} // namespace

bool ChangelogWindow::loadNewEntries(const std::string& sinceVersion) {
    m_sinceVersion = sinceVersion;
    m_entries.clear();

    std::ifstream file("CHANGELOG.md");
    if (!file.is_open()) return false;

    std::string line;
    VersionEntry currentEntry;
    bool collecting = false;
    
    // Improved version regex to handle -alpha, -beta, etc.
    std::regex versionRegex(R"(^## \[([^\]]+)\] - ([0-9-]+))");
    std::smatch match;

    const ParsedVersion sinceParsed = parseVersion(sinceVersion);

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, versionRegex)) {
            std::string version = match[1];

            if (collecting) {
                m_entries.push_back(currentEntry);
            }

            bool includeEntry = true;
            if (sinceVersion != "0.0.0") {
                if (version == sinceVersion) {
                    break;
                }

                const ParsedVersion currentParsed = parseVersion(version);
                if (sinceParsed.valid && currentParsed.valid) {
                    includeEntry = compareVersions(currentParsed, sinceParsed) > 0;
                    if (!includeEntry) {
                        break;
                    }
                }
            }

            currentEntry = {version, match[2], {}};
            collecting = includeEntry;
        } else if (collecting) {
            if (line.find("###") == 0) {
                currentEntry.changes.push_back(line);
            } else if (line.find("- ") == 0 || line.find("  - ") == 0) {
                currentEntry.changes.push_back(line);
            }
        }
    }

    // Add the last entry if we were still collecting
    if (collecting) {
        m_entries.push_back(currentEntry);
    }

    return !m_entries.empty();
}

void ChangelogWindow::render(bool* p_open) {
    if (!ImGui::Begin("What's New", p_open)) {
        ImGui::End();
        return;
    }

    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Pholio Changelog");
    if (m_sinceVersion != "0.0.0") {
        ImGui::Text("Changes since your last visit (%s):", m_sinceVersion.c_str());
    }
    ImGui::Separator();

    ImGui::BeginChild("ChangelogContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 10), true);
    for (const auto& entry : m_entries) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
        ImGui::Text("Version %s", entry.version.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", entry.date.c_str());
        ImGui::Separator();
        
        for (const auto& change : entry.changes) {
            if (change.find("###") == 0) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", change.substr(4).c_str());
            } else {
                ImGui::BulletText("%s", change.substr(2).c_str());
            }
        }
        ImGui::Spacing();
        ImGui::Spacing();
    }
    ImGui::EndChild();

    if (ImGui::Button("Close", ImVec2(-1, 0))) {
        *p_open = false;
    }

    ImGui::End();
}

} // namespace ui
