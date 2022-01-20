#include "asset_system/prefab_asset.hpp"

#include <json/json.hpp>
// #include "lz4.h"

namespace aln::assets
{
using json = nlohmann::json;

PrefabInfo ReadPrefabInfo(AssetFile* file)
{
    PrefabInfo info;
    json metadata = json::parse(file->metadata);

    for (auto pair : metadata["node_transforms"].items())
    {
        auto value = pair.value();
        info.nodeTransforms[value[0]] = value[1];
    }

    for (auto& [key, value] : metadata["node_names"].items())
    {
        info.nodeNames[value[0]] = value[1];
    }

    for (auto& [key, value] : metadata["node_parents"].items())
    {
        info.nodeParents[value[0]] = value[1];
    }

    std::unordered_map<uint64_t, json> meshnodes = metadata["node_meshes"];

    for (auto pair : meshnodes)
    {
        assets::PrefabInfo::NodeMesh node;

        node.meshPath = pair.second["mesh_path"];
        node.materialPath = pair.second["material_path"];

        info.nodeMeshes[pair.first] = node;
    }

    size_t ntransforms = file->binary.size() / (sizeof(float) * 16);
    info.transforms.resize(ntransforms);

    memcpy(info.transforms.data(), file->binary.data(), file->binary.size());

    return info;
}

AssetFile PackPrefab(const PrefabInfo& info)
{
    json metadata;
    metadata["node_transforms"] = info.nodeTransforms;
    metadata["node_names"] = info.nodeNames;
    metadata["node_parents"] = info.nodeParents;

    std::unordered_map<uint64_t, json> meshindex;
    for (auto pair : info.nodeMeshes)
    {
        json meshnode;
        meshnode["mesh_path"] = pair.second.meshPath;
        meshnode["material_path"] = pair.second.materialPath;
        meshindex[pair.first] = meshnode;
    }

    metadata["node_meshes"] = meshindex;

    // Core file header
    AssetFile file;
    file.type = EAssetType::Prefab;
    file.version = 1;

    file.binary.resize(info.transforms.size() * sizeof(float) * 16);
    memcpy(file.binary.data(), info.transforms.data(), info.transforms.size() * sizeof(float) * 16);

    file.metadata = metadata.dump();

    return file;
}
} // namespace aln::assets