#include "ReportWindow.hpp"
#include "core/Config.hpp"
#include "core/ConfigManager.hpp"
#include "I18n.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace ui {

void ReportWindow::render(bool* p_open) {
    if (!*p_open) return;
    auto tr = [](const char* key, const char* fallback) { return i18n::tr(key, fallback); };

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(tr("report.window.title", "Report Issue / Feature"), p_open)) {
        ImGui::TextWrapped("%s", tr("report.window.intro", "Help us improve Pholio! Use this form to report bugs or suggest new features."));
        ImGui::Separator();
        ImGui::Spacing();

        // Type selection
        const char* types[] = {
            tr("report.type.bug", "Bug"),
            tr("report.type.feature", "Feature Request"),
            tr("report.type.feedback", "General Feedback")
        };
        ImGui::Combo(tr("report.type.label", "Type"), &m_typeIdx, types, IM_ARRAYSIZE(types));

        // Priority
        const char* priorities[] = {
            tr("report.priority.low", "Low"),
            tr("report.priority.medium", "Medium"),
            tr("report.priority.high", "High")
        };
        ImGui::Combo(tr("report.priority.label", "Priority"), &m_priority, priorities, IM_ARRAYSIZE(priorities));

        ImGui::InputText(tr("report.title", "Title"), m_title, IM_ARRAYSIZE(m_title));
        ImGui::InputTextMultiline(tr("report.description", "Description"), m_description, IM_ARRAYSIZE(m_description), ImVec2(-FLT_MIN, 200));
        
        ImGui::InputText(tr("report.email", "Your Email (Optional)"), m_email, IM_ARRAYSIZE(m_email));
        ImGui::SetItemTooltip("%s", tr("report.email.tooltip", "We might contact you if we need more information."));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(tr("report.submit", "SUBMIT REPORT"), ImVec2(150, 40))) {
            if (strlen(m_title) > 0 && strlen(m_description) > 0) {
                core::Report report;
                report.type = static_cast<core::ReportType>(m_typeIdx);
                report.title = m_title;
                report.description = m_description;
                report.email = m_email;
                report.priority = m_priority + 1; // 1: Low, 2: Medium, 3: High

                // Timestamp
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
                report.timestamp = ss.str();

                // Version
                report.appVersion = std::string(core::PROJECT_VERSION);

                // System Info
#ifdef _WIN32
                report.systemInfo = "Windows";
#elif defined(__APPLE__)
                report.systemInfo = "macOS";
#else
                report.systemInfo = "Linux / Unix";
#endif

                if (core::ReportManager::saveReport(report)) {
                    m_showSuccess = true;
                    m_showError = false;
                    reset();
                } else {
                    m_showError = true;
                }
            } else {
                m_showError = true; // Simple validation failure
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(tr("report.cancel", "Cancel"), ImVec2(100, 40))) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(tr("report.export", "EXPORT DIAGNOSTICS"), ImVec2(180, 40))) {
            std::filesystem::path bundlePath;
            std::string error;
            if (core::ReportManager::exportDiagnosticsBundle(bundlePath, error)) {
                m_diagStatus = std::string(tr("report.export.ok", "Diagnostics exported to: ")) + bundlePath.string();
                m_showDiagError = false;
            } else {
                m_diagStatus = std::string(tr("report.export.fail", "Diagnostics export failed: ")) + error;
                m_showDiagError = true;
            }
        }

        if (ImGui::Button(tr("report.open_reports", "Open Reports Folder"), ImVec2(200, 0))) {
            const auto reportsPath = core::ConfigManager::getInstance().getReportsDirectory().string();
#ifdef _WIN32
            const int rc = std::system(("start \"\" \"" + reportsPath + "\"").c_str());
#elif __APPLE__
            const int rc = std::system(("open \"" + reportsPath + "\"").c_str());
#else
            const int rc = std::system(("xdg-open \"" + reportsPath + "\"").c_str());
#endif
            if (rc != 0) {
                m_diagStatus = tr("report.open_reports.fail", "Could not open reports folder.");
                m_showDiagError = true;
            }
        }

        if (!m_diagStatus.empty()) {
            if (m_showDiagError) {
                ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "%s", m_diagStatus.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.2f, 1.0f), "%s", m_diagStatus.c_str());
            }
        }

        if (m_showSuccess) {
            ImGui::OpenPopup(tr("report.success.title", "Success"));
            if (ImGui::BeginPopupModal(tr("report.success.title", "Success"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s", tr("report.success.body", "Your report has been saved locally. Thank you!"));
                if (ImGui::Button(tr("report.ok", "OK"), ImVec2(120, 0))) {
                    m_showSuccess = false;
                    ImGui::CloseCurrentPopup();
                    *p_open = false;
                }
                ImGui::EndPopup();
            }
        }

        if (m_showError) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", tr("report.validation", "Please fill in both Title and Description."));
        }
    }
    ImGui::End();
}

void ReportWindow::reset() {
    memset(m_title, 0, sizeof(m_title));
    memset(m_description, 0, sizeof(m_description));
    memset(m_email, 0, sizeof(m_email));
    m_typeIdx = 0;
    m_priority = 1; // Medium
    m_diagStatus.clear();
    m_showDiagError = false;
}

} // namespace ui
