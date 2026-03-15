#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ui {

class GalleryWindow {
public:
    void render(bool* p_open, bool isPreviewOpen, bool* p_previewOpen);
    bool hasSelection() const;
    std::filesystem::path getSelectedImagePath() const;

private:
    void scanImages();
    void selectImage(int index);
    std::string browseFolder(const std::string& defaultPath);

    std::filesystem::path m_rootFolder;
    std::vector<std::filesystem::path> m_images;
    int m_selectedImageIndex = -1;
    std::string m_scanMessage;
};

} // namespace ui
