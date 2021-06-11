#pragma once

#include <cstdint>
#include <string>

#include "asset_system.hpp"

namespace assets
{

enum class TextureFormat : uint32_t
{
    Unknown,
    RGBA8
};

struct TextureInfo
{
    uint64_t size;
    TextureFormat format;
    CompressionMode compressionMode;
    uint32_t pixelSize[3];
    std::string originalFile;
};

/// @brief Read the json metadata and convert it in a TextInfo struct.
TextureInfo ReadTextureInfo(AssetFile* file);

/// @brief Decompress a texture into a buffer.
/// @param info: Texture info
/// @param sourceBuffer Binary blob of pixel data
void UnpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination);

/// @brief Compress and store a texture into binary format.
AssetFile PackTexture(TextureInfo* info, void* pixelData);

} // namespace assets