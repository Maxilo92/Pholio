#include "GalleryWindow.hpp"

#include "I18n.hpp"
#include "../engine/Scanner.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <imgui.h>
#include <nfd.hpp>
#include <system_error>

namespace ui {
namespace {
void openInExplorer(const std::filesystem::path& path) {
    if (path.empty() || !std::filesystem::exists(path)) {
        return;
    }

#ifdef _WIN32
    std::string command = "explorer \"" + path.string() + "\"";
#elif __APPLE__
    std::string command = "open \"" + path.string() + "\"";
#else
    std::string command = "xdg-open \"" + path.string() + "\"";
#endif
    std::system(command.c_str());
}
} // namespace

void GalleryWindow::render(bool* p_open, bool isPreviewOpen, bool* p_previewOpen) {
    if (!*p_open) {
        return;
    }

    if (ImGui::Begin("Gallery", p_open)) {
        if (ImGui::Button(i18n::tr("gallery.select_folder", "Select Folder"))) {
            const std::string pickedFolder = browseFolder(m_rootFolder.string());
            if (!pickedFolder.empty()) {
                m_rootFolder = pickedFolder;
                scanImages();
            }
        }
        ImGui::SameLine();
        const bool hasRootFolder = !m_rootFolder.empty();
        ImGui::BeginDisabled(!hasRootFolder);
        if (ImGui::Button(i18n::tr("gallery.rescan", "Rescan"))) {
            scanImages();
        }
        ImGui::EndDisabled();

        ImGui::TextDisabled("%s", i18n::tr("gallery.root", "Folder:"));
        if (m_rootFolder.empty()) {
            ImGui::TextWrapped("%s", i18n::tr("gallery.no_folder", "Please select a folder."));
        } else {
            ImGui::TextWrapped("%s", m_rootFolder.string().c_str());
        }

        if (!m_scanMessage.empty()) {
            ImGui::Spacing();
            ImGui::TextWrapped("%s", m_scanMessage.c_str());
        }

        ImGui::Spacing();
        ImGui::TextDisabled("%s", i18n::tr("gallery.preview_target_hint", "Selected images are shown in the 'Image Preview' window."));
        if (!isPreviewOpen) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s", i18n::tr("gallery.preview_closed_hint", "Image Preview is currently closed."));
            if (p_previewOpen != nullptr && ImGui::Button(i18n::tr("gallery.open_preview", "Open Image Preview"))) {
                *p_previewOpen = true;
            }
        }

        if (!m_images.empty()) {
            bool selectionChangedByKeyboard = false;
            const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
            if (windowFocused && !ImGui::GetIO().WantTextInput) {
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) || ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) {
                    selectImage(m_selectedImageIndex - 1);
                    selectionChangedByKeyboard = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) || ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) {
                    selectImage(m_selectedImageIndex + 1);
                    selectionChangedByKeyboard = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_PageUp, false)) {
                    selectImage(m_selectedImageIndex - 10);
                    selectionChangedByKeyboard = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_PageDown, false)) {
                    selectImage(m_selectedImageIndex + 10);
                    selectionChangedByKeyboard = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) {
                    selectImage(0);
                    selectionChangedByKeyboard = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_End, false)) {
                    selectImage(static_cast<int>(m_images.size()) - 1);
                    selectionChangedByKeyboard = true;
                }
            }

            ImGui::Spacing();
            ImGui::Text(i18n::tr("gallery.selected", "Selected: %d / %d"), m_selectedImageIndex + 1, static_cast<int>(m_images.size()));
            ImGui::TextDisabled("%s", i18n::tr("gallery.shortcuts", "Shortcuts: Arrow keys, PgUp/PgDn, Home/End"));

            if (ImGui::Button("<")) {
                selectImage(m_selectedImageIndex - 1);
            }
            ImGui::SameLine();
            if (ImGui::Button(">")) {
                selectImage(m_selectedImageIndex + 1);
            }

            ImGui::BeginChild("GalleryList", ImVec2(320.0f, 0.0f), true);
            for (int index = 0; index < static_cast<int>(m_images.size()); ++index) {
                const bool isSelected = index == m_selectedImageIndex;
                const std::string label = m_images[index].filename().string() + "##gallery-img-" + std::to_string(index);
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    selectImage(index);
                }
                if (ImGui::BeginPopupContextItem()) {
                    selectImage(index);
                    if (ImGui::MenuItem(i18n::tr("gallery.context.open_file", "Open File"))) {
                        openInExplorer(m_images[index]);
                        m_scanMessage = i18n::tr("gallery.context.opened", "Opened in system explorer.");
                    }
                    if (ImGui::MenuItem(i18n::tr("gallery.context.open_folder", "Open Containing Folder"))) {
                        openInExplorer(m_images[index].parent_path());
                        m_scanMessage = i18n::tr("gallery.context.opened", "Opened in system explorer.");
                    }
                    if (ImGui::MenuItem(i18n::tr("gallery.context.copy_path", "Copy Full Path"))) {
                        ImGui::SetClipboardText(m_images[index].string().c_str());
                        m_scanMessage = i18n::tr("gallery.context.copied", "Path copied to clipboard.");
                    }
                    ImGui::EndPopup();
                }
                if (isSelected && selectionChangedByKeyboard) {
                    ImGui::SetScrollHereY(0.5f);
                }
            }
            ImGui::EndChild();
        } else if (!m_rootFolder.empty()) {
            ImGui::Spacing();
            ImGui::TextWrapped("%s", i18n::tr("gallery.no_images", "No images found in the selected folder."));
        }
    }
    ImGui::End();
}

void GalleryWindow::scanImages() {
    m_images.clear();
    m_selectedImageIndex = -1;
    m_scanMessage.clear();

    std::error_code ec;
    if (m_rootFolder.empty()) {
        return;
    }
    if (!std::filesystem::exists(m_rootFolder, ec) || !std::filesystem::is_directory(m_rootFolder, ec)) {
        m_scanMessage = i18n::tr("gallery.folder_invalid", "Selected folder is not accessible.");
        return;
    }

    std::filesystem::recursive_directory_iterator it(
        m_rootFolder,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );
    const std::filesystem::recursive_directory_iterator end;

    while (it != end) {
        if (ec) {
            ec.clear();
            it.increment(ec);
            continue;
        }

        std::error_code fileEc;
        if (it->is_regular_file(fileEc) && engine::Scanner::isImage(it->path())) {
            m_images.push_back(it->path());
        }

        it.increment(ec);
    }

    std::sort(m_images.begin(), m_images.end());

    const char* foundFmt = i18n::tr("gallery.images_found", "Found %d images.");
    char msg[128];
    std::snprintf(msg, sizeof(msg), foundFmt, static_cast<int>(m_images.size()));
    m_scanMessage = msg;

    if (!m_images.empty()) {
        selectImage(0);
    }
}

void GalleryWindow::selectImage(int index) {
    if (m_images.empty()) {
        m_selectedImageIndex = -1;
        return;
    }

    if (index < 0) {
        index = 0;
    }
    if (index >= static_cast<int>(m_images.size())) {
        index = static_cast<int>(m_images.size()) - 1;
    }

    m_selectedImageIndex = index;
}

std::string GalleryWindow::browseFolder(const std::string& defaultPath) {
    nfdchar_t* outPath = nullptr;
    std::string path;
    if (NFD_PickFolder(&outPath, defaultPath.c_str()) == NFD_OKAY && outPath != nullptr) {
        path = outPath;
        NFD_FreePath(outPath);
    }
    return path;
}

bool GalleryWindow::hasSelection() const {
    return m_selectedImageIndex >= 0 && m_selectedImageIndex < static_cast<int>(m_images.size());
}

std::filesystem::path GalleryWindow::getSelectedImagePath() const {
    if (!hasSelection()) {
        return {};
    }
    return m_images[m_selectedImageIndex];
}

} // namespace ui
