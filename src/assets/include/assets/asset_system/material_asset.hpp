#pragma once

#include "asset_system.hpp"

#include <string>
#include <unordered_map>

namespace aln::assets
{

enum class TransparencyMode : uint8_t
{
    Opaque,
    Transparent,
    Masked
};

struct MaterialInfo
{
    std::string baseEffect;
    std::unordered_map<std::string, std::string> textures;
    std::unordered_map<std::string, std::string> customProperties;
    TransparencyMode transparency;
};

MaterialInfo ReadMaterialInfo(AssetFile* file);
AssetFile PackMaterial(MaterialInfo* info);
} // namespace aln::assets