#include "asset_system/skeleton_asset.hpp"

#include <lz4.h>
#include <nlohmann/json.hpp>

namespace aln::assets
{
using json = nlohmann::json;

SkeletonInfo SkeletonConverter::ReadInfo(const AssetFile* file)
{
    SkeletonInfo info;

    json metadata = json::parse(file->m_metadata);

    for (auto& [key, value] : metadata["bone_names"].items())
    {
        info.boneNames.push_back(value);
    }
    for (auto& [key, value] : metadata["bone_parent_indices"].items())
    {
        info.boneParentIndices.push_back(value);
    }

    info.originalFilePath = metadata["original_file_path"];
    info.compressionMode = metadata["compression"];

    return info;
}

void SkeletonConverter::Unpack(const SkeletonInfo* info, const std::vector<std::byte>& srcBuffer, std::byte* dstBuffer)
{
    auto size = info->boneNames.size() * sizeof(Transform);
    LZ4_decompress_safe(
        reinterpret_cast<const char*>(srcBuffer.data()),
        reinterpret_cast<char*>(dstBuffer),
        static_cast<int>(srcBuffer.size()),
        static_cast<int>(size));
}

AssetFile SkeletonConverter::Pack(const SkeletonInfo* info, std::vector<Transform>& referencePose)
{
    AssetFile file;
    file.m_assetTypeID = AssetTypeID("skel"); // TODO: Use StaticMesh::GetStaticTypeID();
    file.m_version = 1;

    assert(referencePose.size() == info->boneNames.size());
    assert(referencePose.size() == info->boneParentIndices.size());

    // Find the worst-case compressed size
    auto referencePoseBufferSize = referencePose.size() * sizeof(Transform);
    size_t maxCompressedSize = LZ4_compressBound(static_cast<int>(referencePoseBufferSize));
    file.m_binary.resize(maxCompressedSize);

    // Compress buffer and copy it into the file struct
    auto compressedSize = LZ4_compress_default(
        reinterpret_cast<char*>((std::byte*) referencePose.data()),
        reinterpret_cast<char*>(file.m_binary.data()),
        static_cast<int>(maxCompressedSize),
        static_cast<int>(referencePoseBufferSize));

    // Resize back to the actual compressed size
    file.m_binary.resize(compressedSize);

    json metadata;
    metadata["bone_names"] = info->boneNames;
    metadata["bone_parent_indices"] = info->boneParentIndices;
    metadata["compression"] = CompressionMode::None;
    metadata["original_file_path"] = info->originalFilePath;

    file.m_metadata = metadata.dump();
    return file;
}
} // namespace aln::assets