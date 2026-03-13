#include "ProgressWindow.hpp"
#include <imgui.h>
#include <implot.h>
#include <string>
#include <numeric>
#include <algorithm>

namespace ui {

ProgressWindow::ProgressWindow(engine::Worker& worker) : m_worker(worker) {
    m_fpsHistory.resize(100, 0.0f);
    m_mbpsHistory.resize(100, 0.0f);
}

void ProgressWindow::updateHistory(float dt) {
    m_timeElapsed += dt;
    const bool isRunning = m_worker.isRunning();

    if (m_workerWasRunning && !isRunning) {
        std::fill(m_fpsHistory.begin(), m_fpsHistory.end(), 0.0f);
        std::fill(m_mbpsHistory.begin(), m_mbpsHistory.end(), 0.0f);
    }
    m_workerWasRunning = isRunning;
    
    // Only update history every 0.1s
    static float timer = 0.0f;
    timer += dt;
    if (timer >= 0.1f) {
        m_fpsHistory.pop_front();
        m_fpsHistory.push_back(m_worker.getFilesPerSecond());
        
        m_mbpsHistory.pop_front();
        m_mbpsHistory.push_back(m_worker.getBytesPerSecond() / (1024.0f * 1024.0f));
        
        timer = 0.0f;
    }
}

void ProgressWindow::render() {
    updateHistory(ImGui::GetIO().DeltaTime);

    if (!ImGui::Begin("Progress & Performance")) {
        ImGui::End();
        return;
    }

    // Status and Controls
    ImGui::Text("Status: %s", m_worker.getStatusMessage().c_str());
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    
    if (m_worker.isRunning()) {
        if (ImGui::Button("Stop", ImVec2(80, 0))) {
            m_worker.stop();
        }
    } else {
        if (ImGui::Button("Start", ImVec2(80, 0))) {
            m_worker.start();
        }
    }

    ImGui::Separator();

    // Progress Bar
    float progress = m_worker.getProgress();
    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", m_worker.getProcessedFiles(), m_worker.getTotalFiles());
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), buf);

    // Metrics
    ImGui::Columns(2, "MetricsColumns", false);
    ImGui::Text("Files processed:");
    ImGui::NextColumn();
    ImGui::Text("%d / %d", m_worker.getProcessedFiles(), m_worker.getTotalFiles());
    ImGui::NextColumn();
    
    ImGui::Text("Data processed:");
    ImGui::NextColumn();
    ImGui::Text("%s / %s", formatSize(m_worker.getProcessedBytes()).c_str(), 
                         formatSize(m_worker.getTotalBytes()).c_str());
    ImGui::NextColumn();

    ImGui::Text("Current speed:");
    ImGui::NextColumn();
    ImGui::Text("%.1f files/s (%.1f MB/s)", m_worker.getFilesPerSecond(), 
                m_worker.getBytesPerSecond() / (1024.0f * 1024.0f));
    ImGui::Columns(1);

    ImGui::Separator();

    std::vector<float> x(100);
    std::iota(x.begin(), x.end(), 0.0f);
    std::vector<float> y_fps(m_fpsHistory.begin(), m_fpsHistory.end());
    std::vector<float> y_mbps(m_mbpsHistory.begin(), m_mbpsHistory.end());

    if (ImPlot::BeginPlot("Files per second", ImVec2(-1, 150))) {
        ImPlot::SetupAxes("Time", "Files/s", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 100, ImGuiCond_Always);
        ImPlot::PlotLine("Files/s", x.data(), y_fps.data(), 100);
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("MB per second", ImVec2(-1, 150))) {
        ImPlot::SetupAxes("Time", "MB/s", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 100, ImGuiCond_Always);
        ImPlot::PlotLine("MB/s", x.data(), y_mbps.data(), 100);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

std::string ProgressWindow::formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024 && i < 4) {
        size /= 1024;
        i++;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %s", size, units[i]);
    return std::string(buf);
}

} // namespace ui
