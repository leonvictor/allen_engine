#include "asset_system/texture_asset.hpp"

#include <json/json.hpp>
#include <lz4.h>

namespace aln::assets
{

using json = nlohmann::json;

TextureInfo ReadTextureInfo(const AssetFile* file)
{
    json metadata = json::parse(file->m_metadata);

    TextureInfo info;
    info.format = metadata["format"];
    info.compressionMode = metadata["compression"];
    info.pixelSize[0] = metadata["width"];
    info.pixelSize[1] = metadata["height"];
    info.size = metadata["buffer_size"];
    info.originalFile = metadata["original_file"];

    return info;
}

void UnpackTexture(const TextureInfo* info, const std::vector<std::byte>& sourceBuffer, std::vector<std::byte>& destinationBuffer)
{
    switch (info->compressionMode)
    {
    case CompressionMode::None:
        destinationBuffer = sourceBuffer;
        break;

    case CompressionMode::LZ4:
    {
        destinationBuffer.resize(info->size);
        LZ4_decompress_safe(
            reinterpret_cast<const char*>(sourceBuffer.data()),
            reinterpret_cast<char*>(destinationBuffer.data()),
            static_cast<int>(sourceBuffer.size()),
            static_cast<int>(destinationBuffer.size()));
        break;
    }

    default:
        break;
    }
}

AssetFile PackTexture(const TextureInfo* info, void* pixelData)
{
    AssetFile file;
    file.m_assetTypeID = AssetTypeID("text"); // TODO: Use StaticMesh::GetStaticTypeID();
    file.m_version = 1;

    json metadata;
    metadata["format"] = info->format;
    metadata["width"] = info->pixelSize[0];
    metadata["height"] = info->pixelSize[1];
    metadata["buffer_size"] = info->size;
    metadata["original_file"] = info->originalFile;

    auto maxCompressedSize = LZ4_compressBound(info->size);
    file.m_binary.resize(maxCompressedSize);

    auto compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(pixelData),
        reinterpret_cast<char*>(file.m_binary.data()),
        static_cast<int>(info->size),
        static_cast<int>(maxCompressedSize));

    file.m_binary.resize(compressedSize);

    metadata["compression"] = CompressionMode::LZ4;

    file.m_metadata = metadata.dump();

    return file;
}
} // namespace aln::assets