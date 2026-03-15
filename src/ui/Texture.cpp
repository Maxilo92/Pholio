#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <exiv2/exiv2.hpp>
#include <exiv2/preview.hpp>
#include <iostream>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <cstdlib>
#endif

namespace ui {
namespace {

unsigned char* loadEmbeddedPreviewWithExiv2(const std::filesystem::path& path, int* width, int* height, int* channels) {
    try {
        auto image = Exiv2::ImageFactory::open(path.string());
        if (!image) {
            return nullptr;
        }

        image->readMetadata();
        Exiv2::PreviewManager previewManager(*image);
        Exiv2::PreviewPropertiesList previews = previewManager.getPreviewProperties();
        if (previews.empty()) {
            return nullptr;
        }

        const Exiv2::PreviewProperties& bestPreview = previews.back(); // Sorted ascending, so last is largest.
        Exiv2::PreviewImage previewImage = previewManager.getPreviewImage(bestPreview);
        Exiv2::DataBuf previewBuffer = previewImage.copy();
        if (previewBuffer.empty()) {
            return nullptr;
        }

        return stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(previewBuffer.c_data()),
            static_cast<int>(previewBuffer.size()),
            width,
            height,
            channels,
            4
        );
    } catch (const Exiv2::Error& e) {
        std::cerr << "Texture preview fallback failed for " << path << ": " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Texture preview fallback failed for " << path << ": " << e.what() << '\n';
    }
    return nullptr;
}

#ifdef __APPLE__
unsigned char* loadWithAppleImageIO(const std::filesystem::path& path, int* width, int* height, int* channels) {
    const std::string pathString = path.string();
    CFURLRef fileUrl = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(pathString.c_str()),
        static_cast<CFIndex>(pathString.size()),
        false
    );
    if (!fileUrl) {
        return nullptr;
    }

    CGImageSourceRef source = CGImageSourceCreateWithURL(fileUrl, nullptr);
    CFRelease(fileUrl);
    if (!source) {
        return nullptr;
    }

    CGImageRef cgImage = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    CFRelease(source);
    if (!cgImage) {
        return nullptr;
    }

    const size_t imageWidth = CGImageGetWidth(cgImage);
    const size_t imageHeight = CGImageGetHeight(cgImage);
    if (imageWidth == 0 || imageHeight == 0) {
        CGImageRelease(cgImage);
        return nullptr;
    }

    unsigned char* pixels = static_cast<unsigned char*>(std::malloc(imageWidth * imageHeight * 4));
    if (!pixels) {
        CGImageRelease(cgImage);
        return nullptr;
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    const CGBitmapInfo bitmapInfo = static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedLast) |
                                    static_cast<CGBitmapInfo>(kCGBitmapByteOrder32Big);
    CGContextRef context = CGBitmapContextCreate(
        pixels,
        imageWidth,
        imageHeight,
        8,
        imageWidth * 4,
        colorSpace,
        bitmapInfo
    );
    CGColorSpaceRelease(colorSpace);

    if (!context) {
        std::free(pixels);
        CGImageRelease(cgImage);
        return nullptr;
    }

    CGContextDrawImage(context, CGRectMake(0, 0, imageWidth, imageHeight), cgImage);
    CGContextRelease(context);
    CGImageRelease(cgImage);

    *width = static_cast<int>(imageWidth);
    *height = static_cast<int>(imageHeight);
    *channels = 4;
    return pixels;
}
#endif

} // namespace

Texture::~Texture() {
    release();
}

bool Texture::loadFromFile(const std::filesystem::path& path) {
    release();

    if (!std::filesystem::exists(path)) return false;

    int channels;
    unsigned char* data = stbi_load(path.string().c_str(), &m_width, &m_height, &channels, 4);
    if (!data) {
        const char* stbiReason = stbi_failure_reason();
#ifdef __APPLE__
        data = loadWithAppleImageIO(path, &m_width, &m_height, &channels);
#endif
        if (!data) {
            data = loadEmbeddedPreviewWithExiv2(path, &m_width, &m_height, &channels);
        }
        if (!data && stbiReason != nullptr) {
            std::cerr << "Texture load failed for " << path << ": " << stbiReason << '\n';
        }
        if (!data) {
            return false;
        }
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
