#include "engine/StructureAnalyzer.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::chrono::system_clock::time_point makeLocalTimePoint(
    int year, int month, int day, int hour, int minute, int second) {
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;
    const std::time_t t = std::mktime(&tm);
    if (t == static_cast<std::time_t>(-1)) {
        throw std::runtime_error("failed to build test timestamp");
    }
    return std::chrono::system_clock::from_time_t(t);
}

engine::MediaMetadata makeMetadata(const std::filesystem::path& path) {
    engine::MediaMetadata metadata;
    metadata.path = path;
    metadata.creationTime = makeLocalTimePoint(2024, 3, 14, 9, 8, 7);
    return metadata;
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testResolvesDateAndTokenTemplate() {
    const engine::StructureAnalyzer analyzer(
        "/archive", "%Y/%m/%d", "{original_name}_{HH}{mm}{ss}.{ext}");

    const auto target = analyzer.generatePath(makeMetadata("/input/IMG_0001.JPG"));
    const std::filesystem::path expected = "/archive/2024/03/14/IMG_0001_090807.JPG";

    require(target == expected,
            "expected '" + expected.string() + "' but got '" + target.string() + "'");
}

void testFallsBackToUnnamedAndPreservesOriginalExtension() {
    const engine::StructureAnalyzer analyzer("/archive", "%Y", "...");

    const auto target = analyzer.generatePath(makeMetadata("/input/IMG_0001.JPG"));
    const std::filesystem::path expected = "/archive/2024/unnamed.JPG";

    require(target == expected,
            "expected '" + expected.string() + "' but got '" + target.string() + "'");
}

void testSanitizesInvalidFilenameCharacters() {
    const engine::StructureAnalyzer analyzer(
        "/archive", "%Y", "{original_name}<>:\\\"/\\\\|?*.{ext}");

    const auto target = analyzer.generatePath(makeMetadata("/input/IMG_0001.JPG"));
    const auto filename = target.filename().string();

    require(filename.rfind("IMG_0001", 0) == 0, "sanitized filename lost original prefix: " + filename);
    require(filename.size() >= 4 && filename.substr(filename.size() - 4) == ".JPG",
            "sanitized filename lost extension: " + filename);
    for (char c : filename) {
        const bool invalid = (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' ||
                              c == '|' || c == '?' || c == '*');
        require(!invalid, "sanitized filename still contains invalid character: " + filename);
    }
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"resolves date and token template", testResolvesDateAndTokenTemplate},
        {"falls back to unnamed and keeps extension", testFallsBackToUnnamedAndPreservesOriginalExtension},
        {"sanitizes invalid filename characters", testSanitizesInvalidFilenameCharacters},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        } catch (const std::exception& ex) {
            ++failures;
            std::cerr << "[FAIL] " << test.first << ": " << ex.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All tests passed (" << tests.size() << ").\n";
    return 0;
}
