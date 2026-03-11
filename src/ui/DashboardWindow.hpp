#pragma once

#include "../engine/Worker.hpp"
#include "Texture.hpp"
#include <string>
#include <filesystem>

namespace ui {

class DashboardWindow {
public:
    DashboardWindow(engine::Worker& worker);
    void render();

private:
    engine::Worker& m_worker;
    
    void renderFolderSelection();
    void renderControls();
    void renderStatus();
    void renderPreview();
    
    std::string browseFolder(const std::string& defaultPath);
    void openFolderInExplorer(const std::filesystem::path& path);

    bool m_shouldBrowseSource = false;
    bool m_shouldBrowseTarget = false;

    Texture m_previewTexture;
    std::filesystem::path m_lastLoadedPath;
};

} // namespace ui
