#pragma once

#include "Types.hpp"
#include <filesystem>

namespace engine {

class StructureAnalyzer {
public:
    explicit StructureAnalyzer(std::filesystem::path baseDestPath,
                               std::string folderPattern = "%Y/%m-%B/%d",
                               std::string filenameTemplate = "{original_filename}");

    /**
     * @brief Generates a target path for a media file based on its creation time.
     * 
     * Pattern: baseDestPath / Custom Pattern / filename
     * If creation time is unknown, it uses the current time or a "Unknown" directory.
     * 
     * @param metadata The media metadata containing creation time and original path.
     * @return The absolute target path.
     */
    [[nodiscard]] std::filesystem::path generatePath(const MediaMetadata& metadata) const;

private:
    std::filesystem::path m_baseDestPath;
    std::string m_folderPattern;
    std::string m_filenameTemplate;
};

} // namespace engine
