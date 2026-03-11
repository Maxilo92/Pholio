#pragma once

#include <filesystem>
#include <string>

namespace engine {

enum class VerificationLevel {
    None,
    SizeOnly,
    Partial,    // First 1MB + size
    Full        // Entire file + size
};

class Verifier {
public:
    /**
     * @brief Calculates a hash for the file at the given path based on verification level.
     * 
     * @param path Path to the file.
     * @param level Verification level (None, SizeOnly, Partial, Full).
     * @return A string representing the hash or size.
     */
    static std::string calculateHash(const std::filesystem::path& path, VerificationLevel level);

    /**
     * @brief Verifies if two files are identical based on the verification level.
     * 
     * @param source Source file path.
     * @param destination Destination file path.
     * @param level Verification level.
     * @return true if identical, false otherwise.
     */
    static bool verify(const std::filesystem::path& source, const std::filesystem::path& destination, VerificationLevel level);
};

} // namespace engine
