#include "Worker.hpp"
#include "Scanner.hpp"
#include "MediaAnalyzer.hpp"
#include "StructureAnalyzer.hpp"
#include "Sorter.hpp"
#include "plugins/PluginManager.hpp"
#include "core/ConfigManager.hpp"
#include <iostream>
#include <chrono>
#include <future>
#include <unordered_map>
#include <algorithm>

namespace engine {

namespace {
namespace fs = std::filesystem;

std::string replaceAll(std::string value, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return value;
    }
    size_t startPos = 0;
    while ((startPos = value.find(from, startPos)) != std::string::npos) {
        value.replace(startPos, from.length(), to);
        startPos += to.length();
    }
    return value;
}

std::optional<std::string> buildPatternCandidate(const fs::path& relativeParentPath, const std::tm& tmBuf) {
    const std::string relativeParent = relativeParentPath.generic_string();
    if (relativeParent.empty() || relativeParent == ".") {
        return std::nullopt;
    }

    char yearBuf[5] = {};
    char monthBuf[3] = {};
    char dayBuf[3] = {};
    char monthFullBuf[32] = {};
    char monthShortBuf[8] = {};

    std::strftime(yearBuf, sizeof(yearBuf), "%Y", &tmBuf);
    std::strftime(monthBuf, sizeof(monthBuf), "%m", &tmBuf);
    std::strftime(dayBuf, sizeof(dayBuf), "%d", &tmBuf);
    std::strftime(monthFullBuf, sizeof(monthFullBuf), "%B", &tmBuf);
    std::strftime(monthShortBuf, sizeof(monthShortBuf), "%b", &tmBuf);

    std::string candidate = relativeParent;
    candidate = replaceAll(candidate, monthFullBuf, "%B");
    candidate = replaceAll(candidate, monthShortBuf, "%b");
    candidate = replaceAll(candidate, yearBuf, "%Y");
    candidate = replaceAll(candidate, monthBuf, "%m");
    candidate = replaceAll(candidate, dayBuf, "%d");

    if (candidate.find('%') == std::string::npos) {
        return std::nullopt;
    }

    return candidate;
}

std::optional<std::string> detectExistingPattern(const fs::path& targetPath, ui::LogWindow& logWindow) {
    if (targetPath.empty() || !fs::exists(targetPath) || !fs::is_directory(targetPath)) {
        return std::nullopt;
    }

    MediaAnalyzer analyzer(logWindow);
    std::unordered_map<std::string, int> patternVotes;
    int analyzedFiles = 0;

    for (const auto& entry : fs::recursive_directory_iterator(targetPath)) {
        if (!entry.is_regular_file() || !Scanner::isMedia(entry.path())) {
            continue;
        }
        if (++analyzedFiles > 400) {
            break;
        }

        MediaMetadata metadata;
        metadata.path = entry.path();
        metadata.fileSize = entry.file_size();
        metadata.type = Scanner::isImage(entry.path()) ? MediaType::Image : MediaType::Video;
        analyzer.analyze(metadata);

        std::error_code relEc;
        fs::path relParent = fs::relative(entry.path().parent_path(), targetPath, relEc);
        if (relEc || relParent.empty()) {
            continue;
        }

        auto timestamp = std::chrono::system_clock::to_time_t(metadata.creationTime);
        std::tm tmBuf{};
#ifdef _WIN32
        localtime_s(&tmBuf, &timestamp);
#else
        localtime_r(&timestamp, &tmBuf);
#endif
        auto candidate = buildPatternCandidate(relParent, tmBuf);
        if (!candidate) {
            continue;
        }
        patternVotes[*candidate]++;
    }

    if (patternVotes.empty()) {
        return std::nullopt;
    }

    const auto winner = std::max_element(patternVotes.begin(), patternVotes.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    if (winner == patternVotes.end() || winner->second < 2) {
        return std::nullopt;
    }

    logWindow.info("Detected existing target structure pattern: " + winner->first);
    return winner->first;
}

void runRebuildMigration(const core::AppSettings& settings, ui::LogWindow& logWindow, std::atomic<bool>& shouldStop) {
    if (settings.targetPath.empty() || !fs::exists(settings.targetPath) || !fs::is_directory(settings.targetPath)) {
        return;
    }

    logWindow.info("Migration mode 'Umbau': scanning existing target files for restructure.");
    Scanner scanner(settings.targetPath, logWindow);
    auto existingTasks = scanner.scan();
    if (existingTasks.empty()) {
        logWindow.info("Migration mode 'Umbau': no existing media found in target.");
        return;
    }

    MediaAnalyzer analyzer(logWindow);
    StructureAnalyzer targetAnalyzer(settings.targetPath, settings.folderPattern, settings.filenameTemplate);
    Sorter migrationSorter(logWindow, settings.verificationLevel, settings.duplicateAction, false, {}, settings.dryRun);

    int migrated = 0;
    int unchanged = 0;
    int failed = 0;

    for (auto& task : existingTasks) {
        if (shouldStop) {
            break;
        }

        if (!analyzer.analyze(task.metadata)) {
            failed++;
            continue;
        }

        task.targetPath = targetAnalyzer.generatePath(task.metadata);

        std::error_code eqEc;
        const bool samePath = fs::equivalent(task.metadata.path, task.targetPath, eqEc) ||
                              fs::weakly_canonical(task.metadata.path, eqEc) == fs::weakly_canonical(task.targetPath, eqEc);
        if (samePath) {
            unchanged++;
            continue;
        }

        if (migrationSorter.process(task, OperationMode::Move)) {
            migrated++;
        } else {
            failed++;
        }
    }

    logWindow.info("Migration summary (Umbau): migrated=" + std::to_string(migrated) +
                   ", unchanged=" + std::to_string(unchanged) +
                   ", failed=" + std::to_string(failed));
}
} // namespace

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
    m_lastRunDurationSeconds = 0;
    m_pauseRequested = false;
    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_currentImagePath = "";
    }
    {
        std::lock_guard<std::mutex> lock(m_duplicateMutex);
        m_pendingDuplicatePrompt.reset();
        m_pendingDuplicateDecision.reset();
        m_duplicateDecisionOverride.reset();
    }
    
    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_thread = std::thread(&Worker::run, this);
}

void Worker::stop() {
    m_shouldStop = true;
    m_duplicateCv.notify_all();
    m_pauseCv.notify_all();
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

std::optional<Worker::DuplicatePrompt> Worker::getPendingDuplicatePrompt() const {
    std::lock_guard<std::mutex> lock(m_duplicateMutex);
    return m_pendingDuplicatePrompt;
}

void Worker::submitDuplicateDecision(DuplicateAction action, bool applyToRemaining) {
    {
        std::lock_guard<std::mutex> lock(m_duplicateMutex);
        if (!m_pendingDuplicatePrompt.has_value()) {
            return;
        }
        if (applyToRemaining) {
            m_duplicateDecisionOverride = action;
        }
        m_pendingDuplicateDecision = action;
    }
    m_duplicateCv.notify_all();
}

void Worker::requestPause() {
    m_pauseRequested = true;
}

void Worker::resumeFromPause() {
    m_pauseRequested = false;
    m_pauseCv.notify_all();
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
        auto settings = core::ConfigManager::getInstance().getSettings();
        
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

        // 2. Resolve migration strategy and active folder pattern
        std::optional<std::string> detectedPattern = detectExistingPattern(settings.targetPath, m_logWindow);
        std::string activeFolderPattern = settings.folderPattern;

        if (settings.migrationMode == MigrationMode::Rebuild) {
            runRebuildMigration(settings, m_logWindow, m_shouldStop);
        } else if (settings.migrationMode == MigrationMode::Merge) {
            if (detectedPattern) {
                activeFolderPattern = *detectedPattern;
                m_logWindow.info("Migration mode 'Merge': using detected target pattern for incoming files.");
            } else {
                m_logWindow.info("Migration mode 'Merge': no existing pattern detected, using configured pattern.");
            }
        } else if (settings.migrationMode == MigrationMode::ContinueExisting) {
            if (detectedPattern) {
                activeFolderPattern = *detectedPattern;
                settings.folderPattern = *detectedPattern;
                core::ConfigManager::getInstance().setSettings(settings);
                core::ConfigManager::getInstance().save();
                m_logWindow.info("Migration mode 'Weiterfuehren': adopted detected pattern as new default.");
            } else {
                m_logWindow.info("Migration mode 'Weiterfuehren': no existing pattern detected, using configured pattern.");
            }
        }

        if (m_shouldStop) {
            setStatus("Stopped");
            m_isRunning = false;
            return;
        }

        // 3. Initial Scan to calculate required space
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

        // 4. Check Disk Space
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

        // 5. Start Processing
        MediaAnalyzer analyzer(m_logWindow);
        StructureAnalyzer structAnalyzer(settings.targetPath, activeFolderPattern, settings.filenameTemplate);
        Sorter sorter(
            m_logWindow,
            settings.verificationLevel,
            settings.duplicateAction,
            settings.askOnDuplicate,
            [this](const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath) {
                return requestDuplicateDecision(sourcePath, targetPath);
            },
            settings.dryRun,
            settings.enableFormatConversion,
            settings.imageOutputFormat,
            settings.videoOutputFormat);
        plugins::PluginManager pluginManager(m_logWindow);
        if (settings.enablePlugins) {
            pluginManager.setDisabledPlugins(settings.disabledPlugins);
            pluginManager.loadFromDirectory(settings.pluginsDirectory);
            if (!pluginManager.hasPlugins()) {
                m_logWindow.warn("Plugin system enabled, but no compatible plugins were loaded.");
            }
        }

        auto startTime = std::chrono::steady_clock::now();
        int batchProcessed = 0;
        uint64_t batchBytes = 0;

        // Determine concurrency
        int numThreads = 1;
        if (numThreads > 1) {
            m_logWindow.info("Using parallel processing with " + std::to_string(numThreads) + " threads.");
        }

        auto processTask = [&](MediaTask& task) {
            waitIfPaused();
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
                if (settings.enablePlugins) {
                    std::string skipReason;
                    if (!pluginManager.apply(task, settings.targetPath, task.targetPath, skipReason)) {
                        task.statusMessage = "Skipped by plugin: " + skipReason;
                        m_logWindow.info(task.statusMessage + " [" + task.metadata.path.filename().string() + "]");
                        m_successCount++;
                        m_processedFiles++;
                        m_processedBytes += task.metadata.fileSize;
                        return;
                    }
                }
                
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
                waitIfPaused();
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
                waitIfPaused();
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

    auto finalEndTime = std::chrono::steady_clock::now();
    m_lastRunDurationSeconds = std::chrono::duration_cast<std::chrono::seconds>(finalEndTime - overallStartTime).count();
    m_filesPerSecond = 0.0f;
    m_bytesPerSecond = 0.0f;
    {
        std::lock_guard<std::mutex> lock(m_duplicateMutex);
        m_pendingDuplicatePrompt.reset();
        m_pendingDuplicateDecision.reset();
        m_duplicateDecisionOverride.reset();
    }
    m_isRunning = false;
}

DuplicateAction Worker::requestDuplicateDecision(const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath) {
    {
        std::lock_guard<std::mutex> lock(m_duplicateMutex);
        if (m_duplicateDecisionOverride.has_value()) {
            return *m_duplicateDecisionOverride;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_duplicateMutex);
        m_pendingDuplicatePrompt = DuplicatePrompt{sourcePath, targetPath};
        m_pendingDuplicateDecision.reset();
    }
    setStatus("Duplicate decision required: " + sourcePath.filename().string());
    m_duplicateCv.notify_all();

    std::unique_lock<std::mutex> lock(m_duplicateMutex);
    m_duplicateCv.wait(lock, [this] {
        return m_pendingDuplicateDecision.has_value() || m_shouldStop;
    });

    if (m_shouldStop) {
        m_pendingDuplicatePrompt.reset();
        m_pendingDuplicateDecision.reset();
        return DuplicateAction::Skip;
    }

    const DuplicateAction decision = *m_pendingDuplicateDecision;
    m_pendingDuplicatePrompt.reset();
    m_pendingDuplicateDecision.reset();
    return decision;
}

void Worker::waitIfPaused() {
    if (!m_pauseRequested || m_shouldStop) {
        return;
    }
    setStatus("Paused (waiting for close decision)");
    std::unique_lock<std::mutex> lock(m_pauseMutex);
    m_pauseCv.wait(lock, [this] {
        return !m_pauseRequested || m_shouldStop;
    });
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
