#pragma once

#include "../engine/Worker.hpp"
#include "Texture.hpp"
#include <filesystem>
#include <optional>

namespace ui {

class PreviewWindow {
public:
    PreviewWindow(engine::Worker& worker);
    void render(bool* p_open);
    void setExternalImagePath(const std::filesystem::path& path);
    void clearExternalImagePath();

private:
    engine::Worker& m_worker;
    Texture m_previewTexture;
    std::filesystem::path m_lastLoadedPath;
    std::optional<std::filesystem::path> m_externalImagePath;
};

} // namespace ui
