#include "ReportWindow.hpp"
#include "core/Config.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace ui {

void ReportWindow::render(bool* p_open) {
    if (!*p_open) return;

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Report Issue / Feature", p_open)) {
        ImGui::TextWrapped("Help us improve Pholio! Use this form to report bugs or suggest new features.");
        ImGui::Separator();
        ImGui::Spacing();

        // Type selection
        const char* types[] = { "Bug", "Feature Request", "General Feedback" };
        ImGui::Combo("Type", &m_typeIdx, types, IM_ARRAYSIZE(types));

        // Priority
        const char* priorities[] = { "Low", "Medium", "High" };
        ImGui::Combo("Priority", &m_priority, priorities, IM_ARRAYSIZE(priorities));

        ImGui::InputText("Title", m_title, IM_ARRAYSIZE(m_title));
        ImGui::InputTextMultiline("Description", m_description, IM_ARRAYSIZE(m_description), ImVec2(-FLT_MIN, 200));
        
        ImGui::InputText("Your Email (Optional)", m_email, IM_ARRAYSIZE(m_email));
        ImGui::SetItemTooltip("We might contact you if we need more information.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("SUBMIT REPORT", ImVec2(150, 40))) {
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
                    reset();
                } else {
                    m_showError = true;
                }
            } else {
                m_showError = true; // Simple validation failure
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            *p_open = false;
        }

        if (m_showSuccess) {
            ImGui::OpenPopup("Success");
            if (ImGui::BeginPopupModal("Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Your report has been saved locally. Thank you!");
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    m_showSuccess = false;
                    ImGui::CloseCurrentPopup();
                    *p_open = false;
                }
                ImGui::EndPopup();
            }
        }

        if (m_showError) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Please fill in both Title and Description.");
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
}

} // namespace ui
