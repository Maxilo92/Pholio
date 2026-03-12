#pragma once

#include "Types.hpp"
#include "Verifier.hpp"
#include "core/Loggable.hpp"
#include <filesystem>

namespace engine {

enum class OperationMode {
    Copy,
    Move
};

class Sorter : public core::Loggable {
public:
    explicit Sorter(ui::LogWindow& logWindow, VerificationLevel level = VerificationLevel::Full, 
                    DuplicateAction dupAction = DuplicateAction::Skip, bool askOnDuplicate = false);

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
    DuplicateAction m_dupAction;
    bool m_askOnDuplicate;

    bool handleSidecar(const MediaTask& task, OperationMode mode);
    bool mergeSupplementalMetadata(const MediaTask& task);
};

} // namespace engine
