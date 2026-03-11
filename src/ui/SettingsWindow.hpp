#pragma once

namespace ui {

class SettingsWindow {
public:
    void render();

private:
    bool m_shouldBrowseSource = false;
    bool m_shouldBrowseTarget = false;
};

} // namespace ui
