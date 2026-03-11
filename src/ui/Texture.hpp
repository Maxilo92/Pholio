#pragma once

#include <string>
#include <filesystem>
#include <GLFW/glfw3.h> // More portable than direct GL include

namespace ui {

class Texture {
public:
    Texture() = default;
    ~Texture();

    bool loadFromFile(const std::filesystem::path& path);
    void release();

    GLuint getID() const { return m_id; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool isValid() const { return m_id != 0; }

private:
    GLuint m_id = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace ui
