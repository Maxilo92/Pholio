#pragma once

#include "../engine/Worker.hpp"
#include <string>

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
    
    std::string browseFolder(const std::string& defaultPath);
    void openFolderInExplorer(const std::filesystem::path& path);
};

} // namespace ui
