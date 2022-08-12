#pragma once

#include <string>
#include <vector>

/// TODO: Asset conversion and loading is all over the place. Redesign it so things are neatly organized
namespace aln::assets
{

enum class EAssetType : char
{
    StaticMesh,
    SkeletalMesh,
    Texture,
    Material,
    Animation,
    Prefab,
    Skeleton,
};

enum class CompressionMode : uint8_t
{
    None,
    LZ4
};

/// @brief Intermediary format used to save/load any assets. Contains both metadata and data.
struct AssetFile
{
    EAssetType type;               // Asset type (MESH, TEXT, ANIM...)
    uint32_t version;              // Asset format version
    std::string metadata;          // Asset-type dependent metadata, json format
    std::vector<std::byte> binary; // Asset data in binary format
};

/// @brief Save an asset in binary format.
/// @param path: File to save to
/// @param file: Asset file to save
bool SaveBinaryFile(const std::string& path, const AssetFile& file);

/// @brief Load an asset from binary format.
bool LoadBinaryFile(const std::string& path, AssetFile& outputFile);
} // namespace aln::assets