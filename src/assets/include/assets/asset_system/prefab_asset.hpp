#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "asset_system.hpp"

namespace aln::assets
{

struct PrefabInfo
{
    struct NodeMeshInfo
    {
        std::string materialPath;
        std::string meshPath;
    };

    // TODO: move nodeTransforms, nodeNames, nodeParent and nodeMeshes to a common NodeInfo struct
    std::unordered_map<uint64_t, int> nodeTransforms;      // Map a node index to its transform index in the binary blob
    std::unordered_map<uint64_t, std::string> nodeNames;   // Map a node index to its name
    std::unordered_map<uint64_t, uint64_t> nodeParents;    // Map a node index to its parent index
    std::unordered_map<uint64_t, NodeMeshInfo> nodeMeshes; // Map a node index to its mesh info

    std::vector<std::array<float, 16>> transforms;
};

PrefabInfo ReadPrefabInfo(AssetFile* file);
AssetFile PackPrefab(const PrefabInfo& info);
} // namespace aln::assets