#include "ChangelogWindow.hpp"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace ui {

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

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, versionRegex)) {
            std::string version = match[1];
            
            if (version == sinceVersion && sinceVersion != "0.0.0") {
                // We reached the last known version, stop here
                if (collecting) {
                    m_entries.push_back(currentEntry);
                }
                collecting = false;
                break;
            }

            if (collecting) {
                m_entries.push_back(currentEntry);
            }

            currentEntry = {version, match[2], {}};
            collecting = true;
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
