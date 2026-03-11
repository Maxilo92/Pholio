#include "Worker.hpp"
#include "Scanner.hpp"
#include "MediaAnalyzer.hpp"
#include "StructureAnalyzer.hpp"
#include "Sorter.hpp"
#include "../core/ConfigManager.hpp"
#include <iostream>
#include <chrono>

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
    m_processedBytes = 0;
    m_totalFiles = 0;
    m_totalBytes = 0;
    m_filesPerSecond = 0;
    m_bytesPerSecond = 0;
    
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

void Worker::setStatus(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    m_statusMessage = message;
}

void Worker::run() {
    try {
        const auto& settings = core::ConfigManager::getInstance().getSettings();
        
        m_logWindow.info("Starting background worker process...");
        m_logWindow.info("Source: " + settings.sourcePath.string());
        m_logWindow.info("Target: " + settings.targetPath.string());

        setStatus("Scanning...");
        Scanner scanner(settings.sourcePath);
        auto tasks = scanner.scan();
        
        m_totalFiles = static_cast<int>(tasks.size());
        m_logWindow.info("Found " + std::to_string(m_totalFiles) + " files to process.");

        if (m_totalFiles == 0) {
            m_logWindow.warn("No media files found in source directory.");
            setStatus("Finished (No files)");
            m_isRunning = false;
            return;
        }

        MediaAnalyzer analyzer;
        StructureAnalyzer structAnalyzer(settings.targetPath);
        Sorter sorter(settings.verificationLevel);

        auto startTime = std::chrono::steady_clock::now();
        int batchProcessed = 0;
        uint64_t batchBytes = 0;

        for (auto& task : tasks) {
            if (m_shouldStop) {
                m_logWindow.warn("Processing cancelled by user.");
                setStatus("Cancelled");
                break;
            }

            setStatus("Analyzing: " + task.metadata.path.filename().string());
            
            if (analyzer.analyze(task.metadata)) {
                task.targetPath = structAnalyzer.generatePath(task.metadata);
                
                setStatus("Sorting: " + task.metadata.path.filename().string());
                
                if (sorter.process(task, settings.operationMode)) {
                    m_logWindow.success("Processed: " + task.metadata.path.filename().string() + 
                                       " -> " + task.targetPath.string());
                } else {
                    m_logWindow.error("Failed: " + task.metadata.path.filename().string() + 
                                     " (" + task.statusMessage + ")");
                }
            } else {
                m_logWindow.error("Analysis failed: " + task.metadata.path.filename().string());
            }

            m_processedFiles++;
            m_processedBytes += task.metadata.fileSize;
            batchProcessed++;
            batchBytes += task.metadata.fileSize;

            // Update metrics every 5 files or so
            if (batchProcessed >= 5) {
                updatePerformanceMetrics(batchProcessed, batchBytes, startTime);
                batchProcessed = 0;
                batchBytes = 0;
                startTime = std::chrono::steady_clock::now();
            }
        }

        if (!m_shouldStop) {
            m_logWindow.success("Process completed successfully. " + 
                               std::to_string(m_processedFiles) + "/" + 
                               std::to_string(m_totalFiles) + " files processed.");
            setStatus("Completed");
        }

    } catch (const std::exception& e) {
        m_logWindow.error("Critical error in worker thread: " + std::string(e.what()));
        setStatus("Error: " + std::string(e.what()));
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
