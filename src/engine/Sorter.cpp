#include "Sorter.hpp"
#include <filesystem>
#include <exiv2/exiv2.hpp>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>

namespace engine {

Sorter::Sorter(ui::LogWindow& logWindow, VerificationLevel level, DuplicateAction dupAction, bool askOnDuplicate) 
    : core::Loggable(logWindow), m_level(level), m_dupAction(dupAction), m_askOnDuplicate(askOnDuplicate) {}

bool Sorter::process(MediaTask& task, OperationMode mode) {
    namespace fs = std::filesystem;
    std::error_code ec;

    // 1. Create target directory
    fs::path targetDir = task.targetPath.parent_path();
    if (!fs::exists(targetDir)) {
        if (!fs::create_directories(targetDir, ec)) {
            task.statusMessage = "Failed to create target directory: " + ec.message();
            error("Sorter: Could not create directory: " + targetDir.string() + " Error: " + ec.message());
            return false;
        }
    }

    // Check for duplicates
    if (fs::exists(task.targetPath)) {
        if (m_askOnDuplicate) {
            // Note: In a real app we'd trigger a UI callback or modal
            // For now we treat it as 'Wait for manual interaction' or skip
            task.statusMessage = "Duplicate found! User interaction required (skipping for now)";
            warn("Sorter: Duplicate " + task.targetPath.filename().string() + " found. User interaction requested.");
            return false;
        }

        if (m_dupAction == DuplicateAction::Skip) {
            task.statusMessage = "Skipped (Duplicate exists)";
            info("Sorter: Skipping " + task.metadata.path.filename().string() + " - already exists in target.");
            return true; // Return true because it's a valid outcome
        } else if (m_dupAction == DuplicateAction::Rename) {
            std::string stem = task.targetPath.stem().string();
            std::string ext = task.targetPath.extension().string();
            int counter = 1;
            fs::path newPath = task.targetPath;
            while (fs::exists(newPath)) {
                newPath = targetDir / (stem + " (" + std::to_string(counter++) + ")" + ext);
            }
            task.targetPath = newPath;
            info("Sorter: Renaming duplicate " + stem + ext + " to " + task.targetPath.filename().string());
        } else if (m_dupAction == DuplicateAction::Overwrite) {
            warn("Sorter: Overwriting existing " + task.targetPath.filename().string());
            // No action needed here, copy_file with overwrite_existing will handle it
        }
    }

    // 2. Perform Copy
    // We always copy first to ensure source is safe until verified
    if (!fs::copy_file(task.metadata.path, task.targetPath, fs::copy_options::overwrite_existing, ec)) {
        if (ec == std::errc::no_space_on_device) {
            task.statusMessage = "Disk Full";
        } else {
            task.statusMessage = "Copy failed: " + ec.message();
        }
        error("Sorter: File copy failed from " + task.metadata.path.string() + " to " + task.targetPath.string() + " Error: " + ec.message());
        return false;
    }

    // 3. Verify integrity of the copy
    if (!Verifier::verify(task.metadata.path, task.targetPath, m_level)) {
        task.statusMessage = "Verification failed! Destination file is corrupt or different.";
        error("Sorter: Verification failed for " + task.targetPath.string() + ". Checksums do not match.");
        fs::remove(task.targetPath, ec);
        return false;
    }

    // 4. Merge Supplemental Metadata if available
    if (task.metadata.supplementalMetadataPath) {
        if (mergeSupplementalMetadata(task)) {
            info("Merged supplemental metadata into: " + task.targetPath.filename().string());
        } else {
            warn("Failed to merge supplemental metadata for: " + task.targetPath.filename().string());
        }
    }

    // 5. Handle regular Sidecar
    if (task.metadata.sidecarPath) {
        if (!handleSidecar(task, mode)) {
            task.statusMessage = "Media verified, but sidecar operation failed.";
            warn("Sorter: Sidecar handling failed for " + task.metadata.path.filename().string());
        }
    }

    // 6. Delete Source(s) if Move mode
    if (mode == OperationMode::Move) {
        if (!fs::remove(task.metadata.path, ec)) {
            task.statusMessage = "Verified but failed to remove source: " + ec.message();
            warn("Sorter: Could not remove source file: " + task.metadata.path.string() + " Error: " + ec.message());
        }
        if (task.metadata.supplementalMetadataPath) {
            if (!fs::remove(*task.metadata.supplementalMetadataPath, ec)) {
                warn("Sorter: Could not remove supplemental metadata source: " + task.metadata.supplementalMetadataPath->string() + " Error: " + ec.message());
            }
        }
    }

    task.processed = true;
    if (task.statusMessage.empty() || task.statusMessage.find("Verified but") == std::string::npos) {
        std::string mergeSuffix = task.metadata.supplementalMetadataPath ? " (Merged)" : "";
        task.statusMessage = (mode == OperationMode::Move) ? "Moved and verified" + mergeSuffix : "Copied and verified" + mergeSuffix;
    }
    
    return true;
}

bool Sorter::mergeSupplementalMetadata(const MediaTask& task) {
    if (!task.metadata.supplementalMetadataPath) return true;
    if (task.metadata.type != MediaType::Image) return false; // Exiv2 only for images for now

    try {
        // 1. Read JSON
        std::ifstream file(*task.metadata.supplementalMetadataPath);
        if (!file.is_open()) return false;
        nlohmann::json data;
        file >> data;

        // 2. Open Image
        auto image = Exiv2::ImageFactory::open(task.targetPath.string());
        if (!image) return false;
        image->readMetadata();
        Exiv2::ExifData& exifData = image->exifData();

        // 3. Set Creation Date
        if (data.contains("photoTakenTime") && data["photoTakenTime"].contains("timestamp")) {
            std::string tsStr = data["photoTakenTime"]["timestamp"];
            long long ts = std::stoll(tsStr);
            time_t t = static_cast<time_t>(ts);
            std::tm* tm = std::gmtime(&t); // Exif usually expects local time or UTC without zone, we'll use UTC for simplicity if not specified
            
            std::stringstream ss;
            ss << std::put_time(tm, "%Y:%m:%d %H:%M:%S");
            std::string exifDate = ss.str();

            exifData["Exif.Photo.DateTimeOriginal"] = exifDate;
            exifData["Exif.Photo.DateTimeDigitized"] = exifDate;
            exifData["Exif.Image.DateTime"] = exifDate;
        }

        // 4. Set GPS Data
        if (data.contains("geoData")) {
            double lat = data["geoData"].value("latitude", 0.0);
            double lon = data["geoData"].value("longitude", 0.0);
            double alt = data["geoData"].value("altitude", 0.0);

            if (lat != 0.0 || lon != 0.0) {
                // Simplified GPS writing (Exiv2 handles rational conversion)
                auto toRational = [](double val) {
                    double deg = std::abs(std::floor(val));
                    double min = std::floor((std::abs(val) - deg) * 60.0);
                    double sec = ((std::abs(val) - deg) * 60.0 - min) * 60.0;
                    
                    std::stringstream ss;
                    ss << static_cast<long>(deg) << "/1 " << static_cast<long>(min) << "/1 " << static_cast<long>(sec * 1000) << "/1000";
                    return ss.str();
                };

                exifData["Exif.GPSInfo.GPSVersionID"] = "2 3 0 0";
                exifData["Exif.GPSInfo.GPSLatitudeRef"] = (lat >= 0) ? "N" : "S";
                exifData["Exif.GPSInfo.GPSLatitude"] = toRational(lat);
                exifData["Exif.GPSInfo.GPSLongitudeRef"] = (lon >= 0) ? "E" : "W";
                exifData["Exif.GPSInfo.GPSLongitude"] = toRational(lon);
                exifData["Exif.GPSInfo.GPSAltitudeRef"] = (alt >= 0) ? "0" : "1";
                exifData["Exif.GPSInfo.GPSAltitude"] = std::to_string(static_cast<long>(std::abs(alt))) + "/1";
            }
        }

        // 5. Write back
        image->setExifData(exifData);
        image->writeMetadata();
        return true;
    } catch (const std::exception& e) {
        error("Exiv2 error during merge for " + task.targetPath.filename().string() + ": " + e.what());
        return false;
    }
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
        error("Sorter: Sidecar copy failed for " + sourceSidecar.string() + " Error: " + ec.message());
        return false;
    }

    if (mode == OperationMode::Move) {
        fs::remove(sourceSidecar, ec);
    }
    return true;
}

} // namespace engine
