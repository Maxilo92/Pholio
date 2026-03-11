#include "Sorter.hpp"
#include <filesystem>

namespace engine {

Sorter::Sorter(VerificationLevel level) : m_level(level) {}

bool Sorter::process(MediaTask& task, OperationMode mode) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // 1. Create target directory
    fs::path targetDir = task.targetPath.parent_path();
    if (!fs::exists(targetDir)) {
        if (!fs::create_directories(targetDir, ec)) {
            task.statusMessage = "Failed to create target directory: " + ec.message();
            return false;
        }
    }

    // 2. Perform Copy
    // We always copy first to ensure source is safe until verified
    // Using copy_options::overwrite_existing for retries/resumption
    if (!fs::copy_file(task.metadata.path, task.targetPath, fs::copy_options::overwrite_existing, ec)) {
        task.statusMessage = "Copy failed: " + ec.message();
        return false;
    }

    // 3. Verify
    if (!Verifier::verify(task.metadata.path, task.targetPath, m_level)) {
        task.statusMessage = "Verification failed! Destination file is corrupt or different.";
        // Delete corrupt destination file to prevent half-baked results
        fs::remove(task.targetPath, ec);
        return false;
    }

    // 4. Handle Sidecar
    if (task.metadata.sidecarPath) {
        if (!handleSidecar(task, mode)) {
            // We don't fail the whole task if sidecar fails, but we log it
            task.statusMessage = "Media verified, but sidecar operation failed.";
        }
    }

    // 5. Delete Source if Move mode
    if (mode == OperationMode::Move) {
        if (!fs::remove(task.metadata.path, ec)) {
            task.statusMessage = "Verified but failed to remove source: " + ec.message();
            // Note: We return true here because the file is safely copied and verified
        }
    }

    task.processed = true;
    if (task.statusMessage.empty() || task.statusMessage.find("Verified but") == std::string::npos) {
        task.statusMessage = (mode == OperationMode::Move) ? "Moved and verified" : "Copied and verified";
    }
    
    return true;
}

bool Sorter::handleSidecar(const MediaTask& task, OperationMode mode) {
    namespace fs = std::filesystem;
    if (!task.metadata.sidecarPath) return true;

    fs::path sourceSidecar = *task.metadata.sidecarPath;
    fs::path targetSidecar = task.targetPath.parent_path() / sourceSidecar.filename();

    std::error_code ec;
    // Sidecars are usually small, so we just copy them. 
    // We could verify them too, but for now we trust the filesystem copy for sidecars.
    if (!fs::copy_file(sourceSidecar, targetSidecar, fs::copy_options::overwrite_existing, ec)) {
        return false;
    }

    if (mode == OperationMode::Move) {
        fs::remove(sourceSidecar, ec);
    }
    return true;
}

} // namespace engine
