#include "asset_system/animation_clip_asset.hpp"

#include <nlohmann/json.hpp>
#include <lz4.h>

namespace aln::assets
{

using json = nlohmann::json;

AnimationClipInfo ReadAnimationClipInfo(const AssetFile* file)
{
    json metadata = json::parse(file->m_metadata);

    AnimationClipInfo info;
    info.name = metadata["name"];
    info.duration = metadata["duration"];
    info.framesPerSecond = metadata["fps"];
    info.skeletonID = metadata["skeleton_id"];
    info.framesPerSecond = metadata["fps"];
    info.binaryCompressionMode = metadata["binary_compression"];
    info.originalFile = metadata["original_file"];
    info.binaryBufferSize = metadata["buffer_size"];

    for (auto& [key, value] : metadata["tracks"].items())
    {
        TrackInfo trackInfo;
        trackInfo.boneName = value["bone_name"];
        trackInfo.numTranslationKeys = value["num_translation_keys"];
        trackInfo.numRotationKeys = value["num_rotation_keys"];
        trackInfo.numScaleKeys = value["num_scale_keys"];
        trackInfo.indexInBuffer = value["index_in_buffer"];

        info.tracks.push_back(trackInfo);
    }

    return info;
}

void UnpackAnimationClip(const AnimationClipInfo* info, const std::vector<std::byte>& sourceBuffer, std::vector<float>& destinationBuffer)
{
    // TODO: Decompress to Engine AnimationClip data class directly
    // Where should this happen ?
    destinationBuffer.resize(info->binaryBufferSize);
    if (info->binaryCompressionMode == CompressionMode::LZ4)
    {
        LZ4_decompress_safe(
            reinterpret_cast<const char*>(sourceBuffer.data()),
            reinterpret_cast<char*>(destinationBuffer.data()),
            static_cast<int>(sourceBuffer.size()),
            static_cast<int>(destinationBuffer.size() * sizeof(float)));
    }
}

AssetFile PackAnimationClip(const AnimationClipInfo* info, std::vector<float>& data)
{
    AssetFile file;
    file.m_assetTypeID = AssetTypeID("anim"); // TODO: Use StaticMesh::GetStaticTypeID()
    file.m_version = 1;

    // Compress binary
    // Find the worst-case compressed size
    size_t dataBinarySize = data.size() * sizeof(float);
    size_t maxCompressedSize = LZ4_compressBound(static_cast<int>(dataBinarySize));
    file.m_binary.resize(maxCompressedSize);

    // Compress buffer and copy it into the file struct
    int compressedSize = LZ4_compress_default(
        reinterpret_cast<char*>(data.data()),
        reinterpret_cast<char*>(file.m_binary.data()),
        static_cast<int>(dataBinarySize),
        static_cast<int>(maxCompressedSize));

    // Resize back to the actual compressed size
    file.m_binary.resize(compressedSize);

    std::vector<json> tracksMetadata;
    tracksMetadata.reserve(info->tracks.size());
    for (auto& trackInfo : info->tracks)
    {
        json track;
        track["bone_name"] = trackInfo.boneName;
        track["num_translation_keys"] = trackInfo.numTranslationKeys;
        track["num_rotation_keys"] = trackInfo.numRotationKeys;
        track["num_scale_keys"] = trackInfo.numScaleKeys;
        track["index_in_buffer"] = trackInfo.indexInBuffer;

        tracksMetadata.push_back(track);
    }

    json metadata;
    metadata["name"] = info->name;
    metadata["duration"] = info->duration;
    metadata["fps"] = info->framesPerSecond;
    metadata["skeleton_id"] = info->skeletonID;
    metadata["tracks"] = tracksMetadata;
    metadata["binary_compression"] = CompressionMode::LZ4;
    metadata["buffer_size"] = data.size();
    metadata["original_file"] = info->originalFile;

    file.m_metadata = metadata.dump();
    // TODO: Compression is handled by a compressor class ?
    return file;
}
} // namespace aln::assets