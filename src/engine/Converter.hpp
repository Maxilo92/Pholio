#pragma once

#include <filesystem>
#include <string>

namespace engine {

class Converter {
public:
    static bool isImageToolAvailable();
    static bool isVideoToolAvailable();

    static bool convertImage(const std::filesystem::path& input, const std::filesystem::path& output);
    static bool convertVideo(const std::filesystem::path& input, const std::filesystem::path& output);

private:
    static bool isCommandAvailable(const std::string& command);
    static std::string shellQuote(const std::filesystem::path& path);
};

} // namespace engine
