#pragma once

#include "Types.hpp"
#include "core/Loggable.hpp"
#include <filesystem>
#include <chrono>
#include <nlohmann/json.hpp>

namespace engine {

class MediaAnalyzer : public core::Loggable {
public:
    explicit MediaAnalyzer(ui::LogWindow& logWindow);

    bool analyze(MediaMetadata& metadata);

private:
    bool analyzeImage(MediaMetadata& metadata);
    bool analyzeVideo(MediaMetadata& metadata);
    bool analyzeSupplementalMetadata(MediaMetadata& metadata);
    
    std::chrono::system_clock::time_point getFileModificationTime(const std::filesystem::path& path);
};

} // namespace engine
