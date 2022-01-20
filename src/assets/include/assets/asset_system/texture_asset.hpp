#pragma once

#include <cstdint>
#include <string>

#include "asset_system.hpp"

namespace aln::assets
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
/// @param destinationBuffer Binary buffer to extract to
void UnpackTexture(const TextureInfo* info, const std::vector<std::byte>& sourceBuffer, std::vector<std::byte>& destinationBuffer);

/// @brief Compress and store a texture into binary format.
AssetFile PackTexture(TextureInfo* info, void* pixelData);

TextureFormat ParseFormat(std::string formatString);
} // namespace aln::assets