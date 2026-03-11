#pragma once

#include "Types.hpp"
#include <filesystem>
#include <chrono>

namespace engine {

class MediaAnalyzer {
public:
    MediaAnalyzer() = default;

    bool analyze(MediaMetadata& metadata);

private:
    bool analyzeImage(MediaMetadata& metadata);
    bool analyzeVideo(MediaMetadata& metadata);
    
    std::chrono::system_clock::time_point getFileModificationTime(const std::filesystem::path& path);
};

} // namespace engine
