#pragma once

#include "../ui/LogWindow.hpp"
#include "Types.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <mutex>

namespace engine {

class Worker {
public:
    explicit Worker(ui::LogWindow& logWindow);
    ~Worker();

    void start();
    void stop();
    bool isRunning() const { return m_isRunning; }

    // Progress metrics
    int getTotalFiles() const { return m_totalFiles; }
    int getProcessedFiles() const { return m_processedFiles; }
    uint64_t getTotalBytes() const { return m_totalBytes; }
    uint64_t getProcessedBytes() const { return m_processedBytes; }
    float getFilesPerSecond() const { return m_filesPerSecond; }
    float getBytesPerSecond() const { return m_bytesPerSecond; }
    float getProgress() const;
    std::string getStatusMessage() const;
    std::filesystem::path getCurrentImagePath() const;

private:
    void run();
    void updatePerformanceMetrics(int processedInBatch, uint64_t bytesInBatch, 
                                  std::chrono::steady_clock::time_point startTime);

    ui::LogWindow& m_logWindow;
    std::thread m_thread;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldStop{false};

    std::atomic<int> m_totalFiles{0};
    std::atomic<int> m_processedFiles{0};
    std::atomic<uint64_t> m_totalBytes{0};
    std::atomic<uint64_t> m_processedBytes{0};
    
    std::atomic<float> m_filesPerSecond{0.0f};
    std::atomic<float> m_bytesPerSecond{0.0f};
    
    mutable std::mutex m_statusMutex;
    std::string m_statusMessage{"Idle"};
    std::filesystem::path m_currentImagePath;
    
    void setStatus(const std::string& message);
    void setCurrentImagePath(const std::filesystem::path& path);
};

} // namespace engine
