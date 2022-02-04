#include "asset_system/animation_clip_asset.hpp"

#include <json/json.hpp>
#include <lz4.h>

namespace aln::assets
{

using json = nlohmann::json;

AnimationClipInfo ReadAnimationClipInfo(AssetFile* file)
{
    json metadata = json::parse(file->metadata);

    AnimationClipInfo info;
    info.name = metadata["name"];
    info.duration = metadata["duration"];
    info.framesPerSecond = metadata["fps"];
    info.binaryCompressionMode = metadata["binary_compression"];
    info.originalFile = metadata["original_file"];
    info.bufferSize = metadata["buffer_size"];

    for (auto& [key, value] : metadata["tracks"].items())
    {
        TrackInfo trackInfo;
        trackInfo.boneName = value["bone_name"];
        trackInfo.numKeys = value["num_keys"];

        info.tracks.push_back(trackInfo);
    }

    return info;
}

void UnpackAnimationClip(const AnimationClipInfo* info, const std::vector<std::byte>& sourceBuffer, std::vector<float>& destinationBuffer)
{
    // TODO: Decompress to Engine AnimationClip data class directly
    // Where should this happen ?
    destinationBuffer.resize(info->bufferSize);
    if (info->binaryCompressionMode == CompressionMode::LZ4)
    {
        LZ4_decompress_safe(
            reinterpret_cast<const char*>(sourceBuffer.data()),
            reinterpret_cast<char*>(destinationBuffer.data()),
            static_cast<int>(sourceBuffer.size()),
            static_cast<int>(destinationBuffer.size()));
    }
}

AssetFile PackAnimationClip(AnimationClipInfo* info, std::vector<float>& data)
{
    AssetFile file;
    file.type = EAssetType::Animation;
    file.version = 1;

    // TODO: Compress anim

    // Compress binary
    // Find the worst-case compressed size
    size_t maxCompressedSize = LZ4_compressBound(static_cast<int>(data.size()));
    file.binary.resize(maxCompressedSize);

    // Compress buffer and copy it into the file struct
    int compressedSize = LZ4_compress_default(
        reinterpret_cast<char*>(data.data()),
        reinterpret_cast<char*>(file.binary.data()),
        static_cast<int>(data.size()),
        static_cast<int>(maxCompressedSize));

    // Resize back to the actual compressed size
    file.binary.resize(compressedSize);

    std::vector<json> tracksMetadata;
    tracksMetadata.reserve(info->tracks.size());
    for (auto& trackInfo : info->tracks)
    {
        json track;
        track["bone_name"] = trackInfo.boneName;
        track["num_keys"] = trackInfo.numKeys;

        tracksMetadata.push_back(track);
    }

    json metadata;
    metadata["name"] = info->name;
    metadata["duration"] = info->duration;
    metadata["fps"] = info->framesPerSecond;
    metadata["tracks"] = tracksMetadata;
    metadata["binary_compression"] = CompressionMode::LZ4;
    metadata["buffer_size"] = data.size();
    metadata["original_file"] = info->originalFile;

    file.metadata = metadata.dump();
    // TODO: Compression is handled by a compressor class ?
    return file;
}
} // namespace aln::assets