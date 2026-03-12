#pragma once

#include "Types.hpp"
#include "../core/Loggable.hpp"
#include <vector>
#include <filesystem>
#include <set>
#include <string>

namespace engine {

class Scanner : public core::Loggable {
public:
    explicit Scanner(std::filesystem::path sourceDir, ui::LogWindow& logWindow);

    [[nodiscard]] std::vector<MediaTask> scan();

    static bool isImage(const std::filesystem::path& path);
    static bool isVideo(const std::filesystem::path& path);
    static bool isMedia(const std::filesystem::path& path);

private:
    std::filesystem::path m_sourceDir;
    
    // Set of extensions for quick lookup
    static const std::set<std::string> s_imageExtensions;
    static const std::set<std::string> s_videoExtensions;
    static const std::set<std::string> s_sidecarExtensions;

    void processEntry(const std::filesystem::directory_entry& entry, std::vector<MediaTask>& tasks);
    std::optional<std::filesystem::path> findSidecar(const std::filesystem::path& mediaPath);
    std::optional<std::filesystem::path> findSupplementalMetadata(const std::filesystem::path& mediaPath);
};

} // namespace engine
