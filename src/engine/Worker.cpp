#include "Worker.hpp"
#include "Scanner.hpp"
#include "MediaAnalyzer.hpp"
#include "StructureAnalyzer.hpp"
#include "Sorter.hpp"
#include "core/ConfigManager.hpp"
#include <iostream>
#include <chrono>
#include <future>

namespace engine {

Worker::Worker(ui::LogWindow& logWindow) : m_logWindow(logWindow) {}

Worker::~Worker() {
    stop();
}

void Worker::start() {
    if (m_isRunning) return;
    
    m_isRunning = true;
    m_shouldStop = false;
    m_processedFiles = 0;
    m_successCount = 0;
    m_errorCount = 0;
    m_processedBytes = 0;
    m_totalFiles = 0;
    m_totalBytes = 0;
    m_filesPerSecond = 0;
    m_bytesPerSecond = 0;
    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_currentImagePath = "";
    }
    
    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_thread = std::thread(&Worker::run, this);
}

void Worker::stop() {
    m_shouldStop = true;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_isRunning = false;
}

float Worker::getProgress() const {
    if (m_totalFiles == 0) return 0.0f;
    return static_cast<float>(m_processedFiles) / static_cast<float>(m_totalFiles);
}

std::string Worker::getStatusMessage() const {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_statusMessage;
}

std::filesystem::path Worker::getCurrentImagePath() const {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_currentImagePath;
}

void Worker::setStatus(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_statusMessage = message;
}

void Worker::setCurrentImagePath(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_currentImagePath = path;
}

void Worker::run() {
    auto overallStartTime = std::chrono::steady_clock::now();
    try {
        const auto& settings = core::ConfigManager::getInstance().getSettings();
        
        m_logWindow.info("Starting background worker process...");
        m_logWindow.info("Source: " + settings.sourcePath.string());
        m_logWindow.info("Target: " + settings.targetPath.string());

        // 1. Validate Paths
        if (!std::filesystem::exists(settings.sourcePath)) {
            m_logWindow.error("Source directory does not exist: " + settings.sourcePath.string());
            setStatus("Error: Source not found");
            m_isRunning = false;
            return;
        }

        if (settings.targetPath.empty()) {
            m_logWindow.error("Target directory not specified.");
            setStatus("Error: No target");
            m_isRunning = false;
            return;
        }

        // 2. Initial Scan to calculate required space
        setStatus("Scanning...");
        Scanner scanner(settings.sourcePath, m_logWindow);
        auto tasks = scanner.scan();

        m_totalFiles = static_cast<int>(tasks.size());
        uint64_t totalBytes = 0;
        for (const auto& task : tasks) {
            totalBytes += task.metadata.fileSize;
        }
        m_totalBytes = totalBytes;
        
        m_logWindow.info("Found " + std::to_string(m_totalFiles) + " files to process (" + 
                           std::to_string(totalBytes / (1024 * 1024)) + " MB).");

        if (m_totalFiles == 0) {
            m_logWindow.warn("No media files found in source directory.");
            setStatus("Finished (No files)");
            m_isRunning = false;
            return;
        }

        // 3. Check Disk Space
        // We check the parent path if target doesn't exist yet
        std::filesystem::path checkPath = settings.targetPath;
        while (!checkPath.empty() && !std::filesystem::exists(checkPath)) {
            checkPath = checkPath.parent_path();
        }
        if (checkPath.empty()) checkPath = std::filesystem::current_path();

        std::error_code space_ec;
        auto spaceInfo = std::filesystem::space(checkPath, space_ec);
        if (!space_ec) {
            if (spaceInfo.available < totalBytes) {
                m_logWindow.error("Not enough disk space on target! Required: " + 
                                 std::to_string(totalBytes / (1024 * 1024)) + " MB, Available: " + 
                                 std::to_string(spaceInfo.available / (1024 * 1024)) + " MB");
                setStatus("Error: Disk Full");
                m_isRunning = false;
                return;
            }
        }

        // 4. Start Processing
        MediaAnalyzer analyzer(m_logWindow);
        StructureAnalyzer structAnalyzer(settings.targetPath, settings.folderPattern);
        Sorter sorter(m_logWindow, settings.verificationLevel, settings.duplicateAction, settings.askOnDuplicate);

        auto startTime = std::chrono::steady_clock::now();
        int batchProcessed = 0;
        uint64_t batchBytes = 0;

        // Determine concurrency
        int numThreads = 1;
        if (numThreads > 1) {
            m_logWindow.info("Using parallel processing with " + std::to_string(numThreads) + " threads.");
        }

        auto processTask = [&](MediaTask& task) {
            if (m_shouldStop) return;

            // In parallel mode, we don't update status for every file to avoid flickering
            if (numThreads == 1) {
                setStatus("Analyzing: " + task.metadata.path.filename().string());
            }
            
            if (settings.showPreview && numThreads == 1) {
                setCurrentImagePath(task.metadata.path);
            }
            
            if (analyzer.analyze(task.metadata)) {
                task.targetPath = structAnalyzer.generatePath(task.metadata);
                
                if (numThreads == 1) {
                    setStatus("Sorting: " + task.metadata.path.filename().string());
                }
                
                if (sorter.process(task, settings.operationMode)) {
                    m_logWindow.success("Processed: " + task.metadata.path.filename().string() + 
                                       " -> " + task.targetPath.string());
                    m_successCount++;
                } else {
                    m_logWindow.error("Failed: " + task.metadata.path.filename().string() + 
                                     " (" + task.statusMessage + ")");
                    m_errorCount++;
                }
            } else {
                m_logWindow.error("Analysis failed: " + task.metadata.path.filename().string());
                m_errorCount++;
            }

            m_processedFiles++;
            m_processedBytes += task.metadata.fileSize;
            
            // Note: batchProcessed and batchBytes are harder to track across threads without a mutex
            // but for metrics they don't need to be perfect or we can use atomics
        };

        if (numThreads == 1) {
            for (auto& task : tasks) {
                if (m_shouldStop) break;
                processTask(task);
                
                batchProcessed++;
                batchBytes += task.metadata.fileSize;
                if (batchProcessed >= 5) {
                    updatePerformanceMetrics(batchProcessed, batchBytes, startTime);
                    batchProcessed = 0;
                    batchBytes = 0;
                    startTime = std::chrono::steady_clock::now();
                }
            }
        } else {
            // Parallel processing with a window of 'numThreads'
            std::vector<std::future<void>> futures;
            for (auto& task : tasks) {
                if (m_shouldStop) break;
                
                futures.push_back(std::async(std::launch::async, [&]() { processTask(task); }));
                
                if (futures.size() >= static_cast<size_t>(numThreads)) {
                    // Wait for the oldest one to finish to keep the window size
                    for (auto it = futures.begin(); it != futures.end(); ) {
                        if (it->wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
                            it->get();
                            it = futures.erase(it);
                            
                            // Approximate performance metrics
                            updatePerformanceMetrics(1, 0, startTime); // We don't have bytes easily here
                            startTime = std::chrono::steady_clock::now();
                        } else {
                            ++it;
                        }
                    }
                    // If still full, wait properly for at least one
                    if (futures.size() >= static_cast<size_t>(numThreads)) {
                        futures.front().get();
                        futures.erase(futures.begin());
                    }
                }
            }
            // Wait for remaining
            for (auto& f : futures) f.get();
        }

        if (!m_shouldStop) {
            // Detailed report
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - overallStartTime).count();
            
            float avgFps = (duration > 0) ? static_cast<float>(m_processedFiles) / static_cast<float>(duration) : 0;
            float totalMB = static_cast<float>(m_processedBytes) / (1024.0f * 1024.0f);
            float avgMBps = (duration > 0) ? totalMB / static_cast<float>(duration) : 0;

            m_logWindow.info("-----------------------------------------");
            m_logWindow.info("Summary: " + std::to_string(m_processedFiles) + " files, " + std::to_string(static_cast<int>(totalMB)) + " MB");
            m_logWindow.info("Success: " + std::to_string(m_successCount) + ", Errors: " + std::to_string(m_errorCount));
            m_logWindow.info("Time: " + std::to_string(duration) + "s, Speed: " + std::to_string(avgFps) + " fps (" + std::to_string(avgMBps) + " MB/s)");
            m_logWindow.info("-----------------------------------------");

            std::string summary = "Process finished. " + 
                                 std::to_string(m_successCount) + " successful, " + 
                                 std::to_string(m_errorCount) + " failed, " +
                                 std::to_string(m_totalFiles) + " total.";
            
            if (m_errorCount == 0) {
                m_logWindow.success(summary);
            } else if (m_successCount > 0) {
                m_logWindow.warn(summary);
            } else {
                m_logWindow.error(summary);
            }
            
            setStatus("Completed");
            setCurrentImagePath("");
        }

    } catch (const std::filesystem::filesystem_error& e) {
        std::string errorMsg = e.what();
        if (e.code() == std::errc::no_space_on_device) {
            m_logWindow.error("CRITICAL: Disk full during processing!");
            setStatus("Error: Disk Full");
        } else {
            m_logWindow.error("Filesystem Error: " + errorMsg);
            setStatus("Error: Filesystem");
        }
        setCurrentImagePath("");
    } catch (const std::exception& e) {
        m_logWindow.error("Critical error in worker thread: " + std::string(e.what()));
        setStatus("Error: " + std::string(e.what()));
        setCurrentImagePath("");
    }

    m_isRunning = false;
}

void Worker::updatePerformanceMetrics(int processedInBatch, uint64_t bytesInBatch, 
                                     std::chrono::steady_clock::time_point startTime) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    
    if (duration > 0) {
        float fps = (static_cast<float>(processedInBatch) * 1000.0f) / static_cast<float>(duration);
        float bps = (static_cast<float>(bytesInBatch) * 1000.0f) / static_cast<float>(duration);
        
        m_filesPerSecond = fps;
        m_bytesPerSecond = bps;
    }
}

} // namespace engine
