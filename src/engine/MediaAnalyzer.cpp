#include "MediaAnalyzer.hpp"
#include <exiv2/exiv2.hpp>
#include <iomanip>
#include <sstream>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

namespace engine {

namespace fs = std::filesystem;

bool MediaAnalyzer::analyze(MediaMetadata& metadata) {
    bool success = false;
    if (metadata.type == MediaType::Image) {
        success = analyzeImage(metadata);
    } else if (metadata.type == MediaType::Video) {
        success = analyzeVideo(metadata);
    }

    // Fallback to file modification time if metadata extraction failed or date is missing
    if (!success || metadata.creationTime == std::chrono::system_clock::time_point{}) {
        metadata.creationTime = getFileModificationTime(metadata.path);
    }

    return true; // Always return true because we have a fallback
}

std::chrono::system_clock::time_point parseExifDate(const std::string& dateStr) {
    if (dateStr.empty()) return {};

    std::tm tm = {};
    std::istringstream ss(dateStr);
    
    // 1. Exif format: "YYYY:MM:DD HH:MM:SS"
    if (ss >> std::get_time(&tm, "%Y:%m:%d %H:%M:%S")) {
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
    
    // 2. ISO 8601 variant (FFmpeg): "YYYY-MM-DD HH:MM:SS"
    ss.clear();
    ss.str(dateStr);
    if (ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S")) {
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    // 3. ISO 8601 standard: "YYYY-MM-DDTHH:MM:SS"
    ss.clear();
    ss.str(dateStr);
    if (ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S")) {
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    return {};
}

bool MediaAnalyzer::analyzeImage(MediaMetadata& metadata) {
    try {
        auto image = Exiv2::ImageFactory::open(metadata.path.string());
        if (!image) return false;

        image->readMetadata();
        Exiv2::ExifData& exifData = image->exifData();

        if (exifData.empty()) return false;

        // Try various EXIF date tags
        const char* dateTags[] = {
            "Exif.Photo.DateTimeOriginal",
            "Exif.Photo.DateTimeDigitized",
            "Exif.Image.DateTime"
        };

        for (const auto* tag : dateTags) {
            auto it = exifData.findKey(Exiv2::ExifKey(tag));
            if (it != exifData.end() && it->count() > 0) {
                metadata.creationTime = parseExifDate(it->toString());
                if (metadata.creationTime != std::chrono::system_clock::time_point{}) {
                    return true;
                }
            }
        }
    } catch (const std::exception& e) {
        // Log error if needed: std::cerr << "Exiv2 error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

bool MediaAnalyzer::analyzeVideo(MediaMetadata& metadata) {
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, metadata.path.string().c_str(), nullptr, nullptr) != 0) {
        return false;
    }

    bool found = false;
    if (avformat_find_stream_info(formatCtx, nullptr) >= 0) {
        AVDictionaryEntry* tag = av_dict_get(formatCtx->metadata, "creation_time", nullptr, 0);
        if (tag) {
            metadata.creationTime = parseExifDate(tag->value);
            if (metadata.creationTime != std::chrono::system_clock::time_point{}) {
                found = true;
            }
        }
    }

    avformat_close_input(&formatCtx);
    return found;
}

std::chrono::system_clock::time_point MediaAnalyzer::getFileModificationTime(const fs::path& path) {
    try {
        auto ftime = fs::last_write_time(path);
        
        // Portable conversion for compilers that don't support clock_cast yet (like some AppleClang versions)
        auto s_now = std::chrono::system_clock::now();
        auto f_now = std::filesystem::file_time_type::clock::now();
        return std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - f_now + s_now);
    } catch (...) {
        return std::chrono::system_clock::now();
    }
}

} // namespace engine
