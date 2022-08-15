#include "asset_system/material_asset.hpp"

// #include "lz4.h"

#include <json/json.hpp>

namespace aln::assets
{
using json = nlohmann::json;

MaterialInfo ReadMaterialInfo(const AssetFile* file)
{
    MaterialInfo info;

    json texture_metadata = json::parse(file->m_metadata);
    info.m_name = texture_metadata["name"];
    info.m_albedoMapID = AssetID(texture_metadata["albedo"]);

    return info;
}

AssetFile PackMaterial(const MaterialInfo* info)
{
    nlohmann::json texture_metadata;
    texture_metadata["name"] = info->m_name;
    texture_metadata["albedo"] = info->m_albedoMapID.GetAssetPath();

    // Core file header
    AssetFile file;
    file.m_assetTypeID = AssetTypeID("mtrl"); // TODO: Use StaticMesh::GetStaticTypeID();
    file.m_version = 1;

    file.m_metadata = texture_metadata.dump();

    return file;
}
} // namespace aln::assets
