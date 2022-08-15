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
    std::string m_name;
    AssetID m_albedoMapID;
};

MaterialInfo ReadMaterialInfo(const AssetFile* file);
AssetFile PackMaterial(const MaterialInfo* info);
} // namespace aln::assets