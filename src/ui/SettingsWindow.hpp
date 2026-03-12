#pragma once

#include "../core/ConfigManager.hpp"
#include <future>
#include <filesystem>

namespace ui {

class SettingsWindow {
public:
    void render();

private:
    bool m_shouldBrowseSource = false;
    bool m_shouldBrowseTarget = false;
    
    core::AppSettings m_editedSettings;
    bool m_isDirty = false;
    bool m_initialized = false;

    // Source size calculation
    uint64_t m_sourceSize = 0;
    std::filesystem::path m_lastCalculatedSourcePath;
    std::future<uint64_t> m_sourceSizeFuture;
    bool m_isCalculatingSourceSize = false;
};

} // namespace ui
