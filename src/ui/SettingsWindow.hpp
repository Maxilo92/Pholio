#pragma once

#include "../core/ConfigManager.hpp"

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
};

} // namespace ui
