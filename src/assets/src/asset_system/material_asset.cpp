#include "asset_system/material_asset.hpp"

// #include "lz4.h"

#include <json/json.hpp>

namespace aln::assets
{
using json = nlohmann::json;

MaterialInfo ReadMaterialInfo(AssetFile* file)
{
    MaterialInfo info;

    json texture_metadata = json::parse(file->metadata);
    info.baseEffect = texture_metadata["baseEffect"];

    for (auto& [key, value] : texture_metadata["textures"].items())
    {

        info.textures[key] = value;
    }

    for (auto& [key, value] : texture_metadata["customProperties"].items())
    {

        info.customProperties[key] = value;
    }

    info.transparency = TransparencyMode::Opaque;

    auto it = texture_metadata.find("transparency");
    if (it != texture_metadata.end())
    {
        std::string val = (*it);
        if (val.compare("transparent") == 0)
        {
            info.transparency = TransparencyMode::Transparent;
        }
        if (val.compare("masked") == 0)
        {
            info.transparency = TransparencyMode::Masked;
        }
    }

    return info;
}

AssetFile PackMaterial(MaterialInfo* info)
{
    nlohmann::json texture_metadata;
    texture_metadata["baseEffect"] = info->baseEffect;
    texture_metadata["textures"] = info->textures;
    texture_metadata["customProperties"] = info->customProperties;

    switch (info->transparency)
    {
    case TransparencyMode::Transparent:
        texture_metadata["transparency"] = "transparent";
        break;
    case TransparencyMode::Masked:
        texture_metadata["transparency"] = "masked";
        break;
    }

    // Core file header
    AssetFile file;
    file.type = EAssetType::Material;
    file.version = 1;

    std::string stringified = texture_metadata.dump();
    file.metadata = stringified;

    return file;
}
} // namespace aln::assets
