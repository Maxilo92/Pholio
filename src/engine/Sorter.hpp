#pragma once

#include "Types.hpp"
#include "Verifier.hpp"
#include <filesystem>

namespace engine {

enum class OperationMode {
    Copy,
    Move
};

class Sorter {
public:
    explicit Sorter(VerificationLevel level = VerificationLevel::Full);

    /**
     * @brief Processes a media task: copies or moves the file to the target path,
     * verifies integrity, and deletes source only if verification succeeds in Move mode.
     * 
     * @param task The task to process.
     * @param mode Copy or Move.
     * @return true if successful and verified, false otherwise.
     */
    bool process(MediaTask& task, OperationMode mode);

private:
    VerificationLevel m_level;

    bool handleSidecar(const MediaTask& task, OperationMode mode);
};

} // namespace engine
