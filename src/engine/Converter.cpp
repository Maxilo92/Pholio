#include "Converter.hpp"
#include <cstdlib>

namespace engine {

bool Converter::isImageToolAvailable() {
    return isCommandAvailable("magick");
}

bool Converter::isVideoToolAvailable() {
    return isCommandAvailable("ffmpeg");
}

bool Converter::convertImage(const std::filesystem::path& input, const std::filesystem::path& output) {
    const std::string command = "magick " + shellQuote(input) + " " + shellQuote(output);
    return std::system(command.c_str()) == 0;
}

bool Converter::convertVideo(const std::filesystem::path& input, const std::filesystem::path& output) {
    const std::string command = "ffmpeg -y -loglevel error -i " + shellQuote(input) +
                                " -map_metadata 0 -c:v libx264 -crf 18 -preset medium -c:a aac -b:a 192k " +
                                shellQuote(output);
    return std::system(command.c_str()) == 0;
}

bool Converter::isCommandAvailable(const std::string& command) {
#ifdef _WIN32
    const std::string checkCmd = "where " + command + " >nul 2>&1";
#else
    const std::string checkCmd = "command -v " + command + " >/dev/null 2>&1";
#endif
    return std::system(checkCmd.c_str()) == 0;
}

std::string Converter::shellQuote(const std::filesystem::path& path) {
    std::string value = path.string();
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char c : value) {
        if (c == '\\' || c == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    return "\"" + escaped + "\"";
}

} // namespace engine
