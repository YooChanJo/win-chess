#pragma once
#include "pch.h"
#include <glad/glad.h>

namespace WinChess {
    namespace Graphics {
        enum class ImageFormat {
            R8, GRAY8 = R8, // Gray scale
            RA8, GRAY_ALPHA8 = RA8, // Gray plus alpha
            RG8, RGB8, RGBA8, BGR8, BGRA8
        };
        int GetImageFormatChannels(ImageFormat format);
        GLuint GetGLInternalFormat(ImageFormat format);
        GLuint GetGLFormat(ImageFormat format);
        
        struct ImageData {
            std::vector<uint8_t> Data;
            int Width;
            int Height;
            ImageFormat Format;
        };
        ImageData ReadImageFile(
            const std::filesystem::path& fileName,
            const std::filesystem::path& fileDirectory,
            ImageFormat format,
            bool flip = true
        );
        ImageData ConvertImageFormat(
            const ImageData& srcData,
            ImageFormat dstFormat
        );

        class Texture {
        private:
            GLuint m_GLTexture;
        public:
            Texture(const ImageData& srcData);
            ~Texture();

            inline void Bind() const { glBindTexture(GL_TEXTURE_2D, m_GLTexture); }
            inline void Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

            inline GLuint GetGLTexture() const { return m_GLTexture; }
        };
    }
}