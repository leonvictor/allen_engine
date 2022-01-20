#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "asset_system.hpp"

namespace aln::assets
{

struct PrefabInfo
{
    std::unordered_map<uint64_t, int> nodeTransforms;    // Map a node index to its transform in the binary blob
    std::unordered_map<uint64_t, std::string> nodeNames; // Map a node index to its name
    std::unordered_map<uint64_t, uint64_t> nodeParents;  // Map a node index to its parent index

    struct NodeMesh
    {
        std::string materialPath;
        std::string meshPath;
    };

    std::unordered_map<uint64_t, NodeMesh> nodeMeshes;

    std::vector<std::array<float, 16>> transforms;
};

PrefabInfo ReadPrefabInfo(AssetFile* file);
AssetFile PackPrefab(const PrefabInfo& info);
} // namespace aln::assets