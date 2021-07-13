#include "asset_system/texture_asset.hpp"

#include <json/json.hpp>

using namespace assets;
using json = nlohmann::json;

TextureFormat assets::ParseFormat(std::string formatString)
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

TextureInfo assets::ReadTextureInfo(AssetFile* file)
{
    TextureInfo info;

    json metadata = json::parse(file->json);

    info.format = ParseFormat(metadata["format"]);
    info.compressionMode = ParseCompressionMode(metadata["compression"]);

    info.pixelSize[0] = metadata["width"];
    info.pixelSize[1] = metadata["height"];
    info.size = metadata["buffer_size"];
    info.originalFile = metadata["original_file"];

    return info;
}

void assets::UnpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination)
{
    if (info->compressionMode == CompressionMode::None)
    {
        memcpy(destination, sourceBuffer, sourceSize);
    }
    // TODO: Decompress here
}

// void UnpackTexture(TextureInfo* info, std::vector<char>& sourceBuffer, char* destination)
// {
//     return UnpackTexture(info, sourceBuffer.data(), sourceBuffer.size(), destination);
// }

AssetFile assets::PackTexture(TextureInfo* info, void* pixelData)
{
    json metadata;
    metadata["format"] = "RGBA8";
    metadata["width"] = info->pixelSize[0];
    metadata["height"] = info->pixelSize[1];
    metadata["buffer_size"] = info->size;
    metadata["original_file"] = info->originalFile;

    AssetFile file;
    file.type[0] = 'T';
    file.type[1] = 'E';
    file.type[2] = 'X';
    file.type[3] = 'I';
    file.version = 1;

    // TODO: Compress here
    file.binaryBlob.resize(info->size);
    memcpy(file.binaryBlob.data(), pixelData, info->size);

    metadata["compression"] = "none";

    auto stringified = metadata.dump();
    file.json = stringified;

    return file;
}