#pragma once

#include "../engine/Worker.hpp"
#include "Texture.hpp"
#include <filesystem>

namespace ui {

class PreviewWindow {
public:
    PreviewWindow(engine::Worker& worker);
    void render(bool* p_open);

private:
    engine::Worker& m_worker;
    Texture m_previewTexture;
    std::filesystem::path m_lastLoadedPath;
};

} // namespace ui
