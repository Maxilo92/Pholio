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
    void renderErrorPopup();
    void renderSummaryPopup();
    void renderDuplicatePopup();
    void renderDuplicatePreviewPanel(const char* label, const std::filesystem::path& imagePath, Texture& texture, std::filesystem::path& lastLoadedPath);
    
    std::string browseFolder(const std::string& defaultPath);
    void openFolderInExplorer(const std::filesystem::path& path);

    bool m_shouldBrowseSource = false;
    bool m_shouldBrowseTarget = false;

    std::string m_lastErrorMessage;
    bool m_showErrorPopup = false;
    bool m_showSummaryPopup = false;
    bool m_workerWasRunning = false;

    Texture m_duplicateSourceTexture;
    Texture m_duplicateTargetTexture;
    std::filesystem::path m_lastDuplicateSourceLoadedPath;
    std::filesystem::path m_lastDuplicateTargetLoadedPath;
    bool m_duplicatePopupOpenRequested = false;
    bool m_applyDecisionToRemainingDuplicates = false;
};

} // namespace ui
