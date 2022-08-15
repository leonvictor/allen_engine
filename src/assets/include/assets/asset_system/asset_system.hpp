#pragma once

#include <string>
#include <vector>

#include "../asset_id.hpp"
#include "../asset_type_id.hpp"

/// TODO: Asset conversion and loading is all over the place. Redesign it so things are neatly organized
namespace aln::assets
{

enum class CompressionMode : uint8_t
{
    None,
    LZ4
};

/// @brief Intermediary format used to save/load any assets. Contains both metadata and data.
struct AssetFile
{
    uint32_t m_version;                  // Asset format version
    AssetTypeID m_assetTypeID;           // Asset type (MESH, TEXT, ANIM...)
    std::vector<AssetID> m_dependencies; // Dependencies
    std::string m_metadata;              // Asset-type dependent metadata, json format
    std::vector<std::byte> m_binary;     // Asset data in binary format
};

/// @brief Save an asset in binary format.
/// @param path: File to save to
/// @param file: Asset file to save
bool SaveBinaryFile(const std::string& path, const AssetFile& file);

/// @brief Load an asset from binary format.
bool LoadBinaryFile(const std::string& path, AssetFile& outputFile);
} // namespace aln::assets