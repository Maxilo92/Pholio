#include "engine/StructureAnalyzer.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kDefaultIterations = 200000;

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
    return std::chrono::system_clock::from_time_t(t);
}

std::size_t parseIterations(int argc, char** argv) {
    if (argc < 2) {
        return kDefaultIterations;
    }

    char* end = nullptr;
    const auto parsed = std::strtoull(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || parsed == 0) {
        std::cerr << "Invalid iteration count '" << argv[1]
                  << "'. Falling back to " << kDefaultIterations << ".\n";
        return kDefaultIterations;
    }
    return static_cast<std::size_t>(parsed);
}

} // namespace

int main(int argc, char** argv) {
    const std::size_t iterations = parseIterations(argc, argv);

    const engine::StructureAnalyzer analyzer(
        "/benchmark-archive", "%Y/%m/%d", "{yyyy}-{MM}-{dd}_{original_name}.{ext}");

    std::vector<engine::MediaMetadata> dataset;
    dataset.reserve(iterations);

    for (std::size_t i = 0; i < iterations; ++i) {
        engine::MediaMetadata metadata;
        metadata.path = std::filesystem::path("/input") / ("IMG_" + std::to_string(i) + ".JPG");
        metadata.creationTime = makeLocalTimePoint(
            2024,
            static_cast<int>((i % 12) + 1),
            static_cast<int>((i % 28) + 1),
            static_cast<int>(i % 24),
            static_cast<int>(i % 60),
            static_cast<int>((i * 7) % 60));
        dataset.push_back(std::move(metadata));
    }

    std::size_t outputChecksum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (const auto& metadata : dataset) {
        const auto target = analyzer.generatePath(metadata);
        outputChecksum += target.string().size();
    }
    const auto end = std::chrono::steady_clock::now();

    const auto elapsedNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double elapsedSeconds = static_cast<double>(elapsedNs) / 1'000'000'000.0;
    const double opsPerSecond =
        elapsedSeconds > 0.0 ? static_cast<double>(iterations) / elapsedSeconds : 0.0;
    const double nsPerOp =
        iterations > 0 ? static_cast<double>(elapsedNs) / static_cast<double>(iterations) : 0.0;

    std::cout << "benchmark=StructureAnalyzer.generatePath\n";
    std::cout << "iterations=" << iterations << '\n';
    std::cout << "elapsed_seconds=" << std::fixed << std::setprecision(6) << elapsedSeconds << '\n';
    std::cout << "ops_per_second=" << std::fixed << std::setprecision(2) << opsPerSecond << '\n';
    std::cout << "ns_per_op=" << std::fixed << std::setprecision(2) << nsPerOp << '\n';
    std::cout << "checksum=" << outputChecksum << '\n';

    return 0;
}
