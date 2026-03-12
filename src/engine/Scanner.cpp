#include "Scanner.hpp"
#include <algorithm>
#include <iterator>

namespace engine {

namespace fs = std::filesystem;

const std::set<std::string> Scanner::s_imageExtensions = {
    ".jpg", ".jpeg", ".png", ".tiff", ".tif", ".cr2", ".nef", ".arw", ".dng", ".heic", ".webp"
};

const std::set<std::string> Scanner::s_videoExtensions = {
    ".mp4", ".mov", ".avi", ".mkv", ".m4v", ".wmv", ".flv"
};

const std::set<std::string> Scanner::s_sidecarExtensions = {
    ".xmp", ".xml"
};

Scanner::Scanner(fs::path sourceDir, ui::LogWindow& logWindow) 
    : core::Loggable(logWindow), m_sourceDir(std::move(sourceDir)) {}

std::vector<MediaTask> Scanner::scan() {
    std::vector<MediaTask> tasks;
    if (!fs::exists(m_sourceDir)) {
        error("Source directory does not exist: " + m_sourceDir.string());
        return tasks;
    }
    if (!fs::is_directory(m_sourceDir)) {
        error("Source path is not a directory: " + m_sourceDir.string());
        return tasks;
    }

    info("Scanning for media in: " + m_sourceDir.string());

    for (const auto& entry : fs::recursive_directory_iterator(m_sourceDir)) {
        if (entry.is_regular_file()) {
            processEntry(entry, tasks);
        }
    }
    
    info("Scan complete. Validated " + std::to_string(tasks.size()) + " media items.");
    return tasks;
}

bool Scanner::isImage(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return s_imageExtensions.find(ext) != s_imageExtensions.end();
}

bool Scanner::isVideo(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return s_videoExtensions.find(ext) != s_videoExtensions.end();
}

bool Scanner::isMedia(const fs::path& path) {
    return isImage(path) || isVideo(path);
}

void Scanner::processEntry(const fs::directory_entry& entry, std::vector<MediaTask>& tasks) {
    fs::path path = entry.path();
    if (!isMedia(path)) {
        return;
    }

    MediaMetadata metadata;
    metadata.path = path;
    metadata.fileSize = entry.file_size();
    metadata.format = path.extension().string();
    metadata.type = isImage(path) ? MediaType::Image : MediaType::Video;
    metadata.sidecarPath = findSidecar(path);
    metadata.supplementalMetadataPath = findSupplementalMetadata(path);

    tasks.push_back({metadata, {}, false, ""});
}

std::optional<fs::path> Scanner::findSidecar(const fs::path& mediaPath) {
    // Check for sidecar.ext (e.g., IMG_1234.JPG.xmp)
    fs::path sidecar1 = mediaPath.string() + ".xmp";
    if (fs::exists(sidecar1)) {
        return sidecar1;
    }

    // Check for sidecar with swapped extension (e.g., IMG_1234.xmp)
    fs::path sidecar2 = mediaPath;
    sidecar2.replace_extension(".xmp");
    if (sidecar2 != mediaPath && fs::exists(sidecar2)) {
        return sidecar2;
    }

    return std::nullopt;
}

std::optional<fs::path> Scanner::findSupplementalMetadata(const fs::path& mediaPath) {
    // 1. Check for <filename>.<ext>.supplemental-metadata.json
    fs::path supp1 = mediaPath.string() + ".supplemental-metadata.json";
    if (fs::exists(supp1)) {
        return supp1;
    }

    // 2. Check for <filename>.<ext>.json
    fs::path supp2 = mediaPath.string() + ".json";
    if (fs::exists(supp2)) {
        return supp2;
    }

    // 3. Check for <filename>.json
    fs::path supp3 = mediaPath;
    supp3.replace_extension(".json");
    if (supp3 != mediaPath && fs::exists(supp3)) {
        return supp3;
    }

    return std::nullopt;
}

} // namespace engine
