#include "Texture.h"
#include <stb_image.h>

namespace WinChess {
    namespace Graphics {
        GLuint GetGLInternalFormat(ImageFormat format) {
            switch (format) {
                case ImageFormat::R8: return GL_R8;
                case ImageFormat::RA8: return GL_RG8;
                case ImageFormat::RG8: return GL_RG8;
                case ImageFormat::RGB8: return GL_RGB8;
                case ImageFormat::RGBA8: return GL_RGBA8;
                case ImageFormat::BGR8: return GL_RGB8;
                case ImageFormat::BGRA8: return GL_RGBA8;
                default: throw std::runtime_error("Invalid image format");
            }
        }
        GLuint GetGLFormat(ImageFormat format) {
            switch (format) {
                case ImageFormat::R8: return GL_RED;
                case ImageFormat::RA8: return GL_RG;
                case ImageFormat::RG8: return GL_RG;
                case ImageFormat::RGB8: return GL_RGB;
                case ImageFormat::RGBA8: return GL_RGBA;
                case ImageFormat::BGR8: return GL_BGR;
                case ImageFormat::BGRA8: return GL_BGRA;
                default: throw std::runtime_error("Invalid image format");
            }
        };
        int GetImageFormatChannels(ImageFormat format) {
            switch (format) {
                case ImageFormat::R8:    return 1;
                case ImageFormat::RA8:   return 2;
                case ImageFormat::RG8:   return 2;
                case ImageFormat::RGB8:  return 3;
                case ImageFormat::RGBA8: return 4;
                case ImageFormat::BGR8:  return 3;
                case ImageFormat::BGRA8: return 4;
                default: throw std::runtime_error("Invalid image format");
            }
        }

        ImageData ReadImageFile(
            const std::filesystem::path& fileName,
            const std::filesystem::path& fileDirectory,
            ImageFormat format,
            bool flip
        ) {
            std::filesystem::path fullPath = fileDirectory / fileName;
            if(!std::filesystem::exists(fullPath)) {
                throw std::runtime_error("Image file does not exist: " + fullPath.string());
            }
            int width, height, channels;
            stbi_set_flip_vertically_on_load(flip); // OpenGL expects 0, 0 at bottom-left
            uint8_t* data = stbi_load(fullPath.string().c_str(), &width, &height, &channels, 0);
            if (!data) {
                throw std::runtime_error("Failed to load image file texture data");
            }
            if (channels != GetImageFormatChannels(format)) {
                throw std::runtime_error("Invalid current image format specification");
            }
            int dataSize = width * height * GetImageFormatChannels(format);
            ImageData out;
            out.Width = width;
            out.Height = height;
            out.Format = format;
            out.Data.assign(data, data + dataSize);
            stbi_image_free(data); // Free image memory
            return out;
        }
        ImageData ConvertImageFormat(
            const ImageData& srcData,
            ImageFormat dstFormat
        ) {
            int numPixels = srcData.Width * srcData.Height;
            int dstChannels = GetImageFormatChannels(dstFormat);
            
            ImageData out;
            out.Width = srcData.Width;
            out.Height = srcData.Height;
            out.Format = dstFormat;
            out.Data.resize(numPixels * dstChannels);
            for (int i = 0; i < numPixels; i++) {
                uint8_t r = 0, g = 0, b = 0, a = 255;
                uint8_t gray;
                // Load source pixel
                switch (srcData.Format) {
                    case ImageFormat::R8:
                        r = g = b = srcData.Data[i];
                        a = 255;
                        gray = static_cast<uint8_t>(0.5f * r + 0.5f * g);
                        break;
                    case ImageFormat::RA8:
                        r = g = b = srcData.Data[i * 2 + 0];
                        a = srcData.Data[i * 2 + 1];
                        gray = static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b);
                        break;    
                    case ImageFormat::RG8:
                        r = srcData.Data[i * 2 + 0];
                        g = srcData.Data[i * 2 + 1];
                        b = 0; a = 255;
                        break;
                    case ImageFormat::RGB8:
                        r = srcData.Data[i * 3 + 0];
                        g = srcData.Data[i * 3 + 1];
                        b = srcData.Data[i * 3 + 2];
                        a = 255;
                        break;
                    case ImageFormat::RGBA8:
                        r = srcData.Data[i * 4 + 0];
                        g = srcData.Data[i * 4 + 1];
                        b = srcData.Data[i * 4 + 2];
                        a = srcData.Data[i * 4 + 3];
                        break;
                    case ImageFormat::BGR8:
                        b = srcData.Data[i * 3 + 0];
                        g = srcData.Data[i * 3 + 1];
                        r = srcData.Data[i * 3 + 2];
                        a = 255;
                        break;
                    case ImageFormat::BGRA8:
                        b = srcData.Data[i * 4 + 0];
                        g = srcData.Data[i * 4 + 1];
                        r = srcData.Data[i * 4 + 2];
                        a = srcData.Data[i * 4 + 3];
                        break;
                    default: break;
                }
                gray = static_cast<uint8_t>(
                    srcData.Format == ImageFormat::R8 ?
                    (0.5f * r + 0.5f * g) :
                    (0.299f * r + 0.587f * g + 0.114f * b)
                );
                // Store destination pixel
                switch (dstFormat) {
                    case ImageFormat::R8:
                        out.Data[i] = gray;
                        break;
                    case ImageFormat::RA8:
                        out.Data[i * 2 + 0] = gray;
                        out.Data[i * 2 + 1] = a;
                        break;
                    case ImageFormat::RG8:
                        out.Data[i * 2 + 0] = r;
                        out.Data[i * 2 + 1] = g;
                        break;
                    case ImageFormat::RGB8:
                        out.Data[i * 3 + 0] = r;
                        out.Data[i * 3 + 1] = g;
                        out.Data[i * 3 + 2] = b;
                        break;
                    case ImageFormat::RGBA8:
                        out.Data[i * 4 + 0] = r;
                        out.Data[i * 4 + 1] = g;
                        out.Data[i * 4 + 2] = b;
                        out.Data[i * 4 + 3] = a;
                        break;
                    case ImageFormat::BGR8:
                        out.Data[i * 3 + 0] = b;
                        out.Data[i * 3 + 1] = g;
                        out.Data[i * 3 + 2] = r;
                        break;
                    case ImageFormat::BGRA8:
                        out.Data[i * 4 + 0] = b;
                        out.Data[i * 4 + 1] = g;
                        out.Data[i * 4 + 2] = r;
                        out.Data[i * 4 + 3] = a;
                        break;
                    default: break;
                }
            }
            return out;
        }
        
        Texture::Texture(const ImageData& srcData) {
            glGenTextures(1, &m_GLTexture);
            glBindTexture(GL_TEXTURE_2D, m_GLTexture);
    
            // Set wrapping & filtering options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GetGLInternalFormat(srcData.Format),
                srcData.Width, srcData.Height,
                0,
                GetGLFormat(srcData.Format),
                GL_UNSIGNED_BYTE,
                srcData.Data.data()
            );
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        Texture::~Texture() {
            glDeleteTextures(1, &m_GLTexture);
        }
    }
}