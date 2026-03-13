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
#include <optional>
#include <condition_variable>

namespace engine {

class Worker {
public:
    struct DuplicatePrompt {
        std::filesystem::path sourcePath;
        std::filesystem::path targetPath;
    };
    struct LowDiskSpacePrompt {
        uint64_t requiredBytes = 0;
        uint64_t availableBytes = 0;
        bool moveMode = false;
    };

    explicit Worker(ui::LogWindow& logWindow);
    ~Worker();

    void start();
    void stop();
    bool isRunning() const { return m_isRunning; }

    // Progress metrics
    int getTotalFiles() const { return m_totalFiles; }
    int getProcessedFiles() const { return m_processedFiles; }
    int getSuccessCount() const { return m_successCount; }
    int getErrorCount() const { return m_errorCount; }
    uint64_t getTotalBytes() const { return m_totalBytes; }
    uint64_t getProcessedBytes() const { return m_processedBytes; }
    float getFilesPerSecond() const { return m_filesPerSecond; }
    float getBytesPerSecond() const { return m_bytesPerSecond; }
    int64_t getLastRunDurationSeconds() const { return m_lastRunDurationSeconds; }
    float getProgress() const;
    std::string getStatusMessage() const;
    std::filesystem::path getCurrentImagePath() const;
    std::optional<DuplicatePrompt> getPendingDuplicatePrompt() const;
    std::optional<LowDiskSpacePrompt> getPendingLowDiskSpacePrompt() const;
    void submitDuplicateDecision(DuplicateAction action, bool applyToRemaining);
    void submitLowDiskSpaceDecision(bool shouldContinue);
    void requestPause();
    void resumeFromPause();
    void resetProgress();
    void prepareNewSort();

private:
    void run();
    DuplicateAction requestDuplicateDecision(const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath);
    bool requestLowDiskSpaceDecision(uint64_t requiredBytes, uint64_t availableBytes, bool moveMode);
    void waitIfPaused();
    void updatePerformanceMetrics(int processedInBatch, uint64_t bytesInBatch, 
                                  std::chrono::steady_clock::time_point startTime);

    ui::LogWindow& m_logWindow;
    std::thread m_thread;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldStop{false};

    std::atomic<int> m_totalFiles{0};
    std::atomic<int> m_processedFiles{0};
    std::atomic<int> m_successCount{0};
    std::atomic<int> m_errorCount{0};
    std::atomic<uint64_t> m_totalBytes{0};
    std::atomic<uint64_t> m_processedBytes{0};
    
    std::atomic<float> m_filesPerSecond{0.0f};
    std::atomic<float> m_bytesPerSecond{0.0f};
    std::atomic<int64_t> m_lastRunDurationSeconds{0};
    
    mutable std::mutex m_statusMutex;
    std::string m_statusMessage{"Idle"};
    std::filesystem::path m_currentImagePath;

    mutable std::mutex m_duplicateMutex;
    std::condition_variable m_duplicateCv;
    std::optional<DuplicatePrompt> m_pendingDuplicatePrompt;
    std::optional<DuplicateAction> m_pendingDuplicateDecision;
    std::optional<DuplicateAction> m_duplicateDecisionOverride;
    mutable std::mutex m_lowDiskSpaceMutex;
    std::condition_variable m_lowDiskSpaceCv;
    std::optional<LowDiskSpacePrompt> m_pendingLowDiskSpacePrompt;
    std::optional<bool> m_pendingLowDiskSpaceDecision;

    std::atomic<bool> m_pauseRequested{false};
    std::condition_variable m_pauseCv;
    mutable std::mutex m_pauseMutex;
    
    void setStatus(const std::string& message);
    void setCurrentImagePath(const std::filesystem::path& path);
};

} // namespace engine
