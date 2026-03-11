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

Scanner::Scanner(fs::path sourceDir) : m_sourceDir(std::move(sourceDir)) {}

std::vector<MediaTask> Scanner::scan() {
    std::vector<MediaTask> tasks;
    if (!fs::exists(m_sourceDir) || !fs::is_directory(m_sourceDir)) {
        return tasks;
    }

    for (const auto& entry : fs::recursive_directory_iterator(m_sourceDir)) {
        if (entry.is_regular_file()) {
            processEntry(entry, tasks);
        }
    }
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

} // namespace engine
