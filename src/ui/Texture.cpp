#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

namespace ui {

Texture::~Texture() {
    release();
}

bool Texture::loadFromFile(const std::filesystem::path& path) {
    release();

    if (!std::filesystem::exists(path)) return false;

    int channels;
    unsigned char* data = stbi_load(path.string().c_str(), &m_width, &m_height, &channels, 4);
    if (!data) {
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    stbi_image_free(data);
    return true;
}

void Texture::release() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
        m_width = 0;
        m_height = 0;
    }
}

} // namespace ui
