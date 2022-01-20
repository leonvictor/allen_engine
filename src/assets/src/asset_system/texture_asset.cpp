#include "asset_system/texture_asset.hpp"

#include <json/json.hpp>
#include <lz4.h>

namespace aln::assets
{

using json = nlohmann::json;

TextureFormat ParseFormat(std::string formatString)
{
    if (formatString == "RGBA8")
    {
        return TextureFormat::RGBA8;
    }
    else
    {
        return TextureFormat::Unknown;
    }
};

TextureInfo ReadTextureInfo(AssetFile* file)
{
    TextureInfo info;

    json metadata = json::parse(file->metadata);

    info.format = ParseFormat(metadata["format"]);
    info.compressionMode = ParseCompressionMode(metadata["compression"]);

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

AssetFile PackTexture(TextureInfo* info, void* pixelData)
{
    AssetFile file;
    file.type = EAssetType::Texture;
    file.version = 1;

    json metadata;
    metadata["format"] = "RGBA8";
    metadata["width"] = info->pixelSize[0];
    metadata["height"] = info->pixelSize[1];
    metadata["buffer_size"] = info->size;
    metadata["original_file"] = info->originalFile;

    auto maxCompressedSize = LZ4_compressBound(info->size);
    file.binary.resize(maxCompressedSize);

    auto compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(pixelData),
        reinterpret_cast<char*>(file.binary.data()),
        static_cast<int>(info->size),
        static_cast<int>(maxCompressedSize));

    file.binary.resize(compressedSize);

    metadata["compression"] = "LZ4";

    file.metadata = metadata.dump();

    return file;
}
} // namespace aln::assets