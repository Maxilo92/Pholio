#pragma once

#include <filesystem>
#include <string>
#include <chrono>
#include <vector>
#include <optional>

namespace engine {

enum class MediaType {
    Image,
    Video,
    Unknown
};

enum class DuplicateAction {
    Skip,
    Overwrite,
    Rename
};

enum class MigrationMode {
    Rebuild,
    Merge,
    ContinueExisting
};

struct MediaMetadata {
    std::filesystem::path path;
    MediaType type = MediaType::Unknown;
    std::chrono::system_clock::time_point creationTime;
    std::string format;
    std::uintmax_t fileSize = 0;
    std::optional<std::filesystem::path> sidecarPath;
    std::optional<std::filesystem::path> supplementalMetadataPath;
};

struct MediaTask {
    MediaMetadata metadata;
    std::filesystem::path targetPath;
    bool processed = false;
    std::string statusMessage;
};

} // namespace engine
