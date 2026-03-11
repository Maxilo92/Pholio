#include "Verifier.hpp"
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>

#if __has_include(<xxhash.h>)
#include <xxhash.h>
#define USE_XXHASH
#endif

namespace engine {

std::string Verifier::calculateHash(const std::filesystem::path& path, VerificationLevel level) {
    if (level == VerificationLevel::None) {
        return "";
    }

    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) return "error_io";

    if (level == VerificationLevel::SizeOnly) {
        return std::to_string(size);
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) return "error_open";

#ifdef USE_XXHASH
    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0);

    const size_t bufferSize = 65536; // 64 KB
    std::vector<char> buffer(bufferSize);

    if (level == VerificationLevel::Partial) {
        // Read the first 1 MB
        size_t bytesToRead = std::min<size_t>(size, 1024 * 1024);
        while (bytesToRead > 0 && file.read(buffer.data(), std::min(bufferSize, bytesToRead))) {
            XXH64_update(state, buffer.data(), static_cast<size_t>(file.gcount()));
            bytesToRead -= static_cast<size_t>(file.gcount());
        }
    } else {
        // Full hash
        while (file.read(buffer.data(), bufferSize)) {
            XXH64_update(state, buffer.data(), static_cast<size_t>(file.gcount()));
        }
        if (file.gcount() > 0) {
            XXH64_update(state, buffer.data(), static_cast<size_t>(file.gcount()));
        }
    }

    XXH64_hash_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str() + ":" + std::to_string(size);

#else
    // Robust fallback: simple additive hash of blocks
    // This provides basic data integrity but is not as robust as xxHash
    uint64_t hashValue = size;
    const size_t bufferSize = 65536;
    std::vector<char> buffer(bufferSize);

    if (level == VerificationLevel::Partial) {
        size_t bytesToRead = std::min<size_t>(size, 1024 * 1024);
        while (bytesToRead > 0 && file.read(buffer.data(), std::min(bufferSize, bytesToRead))) {
            for (size_t i = 0; i < static_cast<size_t>(file.gcount()); ++i) {
                hashValue = ((hashValue << 5) + hashValue) + static_cast<unsigned char>(buffer[i]);
            }
            bytesToRead -= static_cast<size_t>(file.gcount());
        }
    } else {
        while (file.read(buffer.data(), bufferSize)) {
            for (size_t i = 0; i < static_cast<size_t>(file.gcount()); ++i) {
                hashValue = ((hashValue << 5) + hashValue) + static_cast<unsigned char>(buffer[i]);
            }
        }
        if (file.gcount() > 0) {
            for (size_t i = 0; i < static_cast<size_t>(file.gcount()); ++i) {
                hashValue = ((hashValue << 5) + hashValue) + static_cast<unsigned char>(buffer[i]);
            }
        }
    }
    
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hashValue;
    return ss.str() + ":" + std::to_string(size);
#endif
}

bool Verifier::verify(const std::filesystem::path& source, const std::filesystem::path& destination, VerificationLevel level) {
    if (level == VerificationLevel::None) return true;
    
    // Always check size first as it's cheap
    std::error_code ec;
    std::uintmax_t s1 = std::filesystem::file_size(source, ec);
    if (ec) return false;
    std::uintmax_t s2 = std::filesystem::file_size(destination, ec);
    if (ec) return false;
    
    if (s1 != s2) return false;
    if (level == VerificationLevel::SizeOnly) return true;

    return calculateHash(source, level) == calculateHash(destination, level);
}

} // namespace engine
