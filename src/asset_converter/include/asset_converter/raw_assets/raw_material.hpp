#pragma once

#include <assert.h>

#include <assets/asset_archive_header.hpp>
#include <common/serialization/binary_archive.hpp>

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"

namespace aln::assets::converter
{
class RawMaterial : public IRawAsset
{
    friend class AssimpMaterialReader;

    AssetID m_albedoMapID;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        archive << m_albedoMapID;
    }
};

class AssimpMaterialReader
{
  public:
    static void ReadMaterial(const AssimpSceneContext& sceneContext, const aiMaterial* pMaterial)
    {
        RawMaterial material;
        auto name = std::string(pMaterial->GetName().C_Str());

        // TODO: What are those empty materials ?
        if (name.empty())
        {
            return;
        }

        // Diffuse
        auto textureCount = pMaterial->GetTextureCount(aiTextureType_DIFFUSE);
        assert(textureCount <= 1); // We only support one texture for now
        for (auto textureIndex = 0; textureIndex < textureCount; ++textureIndex)
        {
            aiString texturePath;
            pMaterial->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr);

            // Transfer texture path to its export equivalent
            // TODO: Handle actual texture export here
            /// @note we have to use .. for now as the exporter creates a new folder for each gltf/fbx file
            auto textureExportPath = std::filesystem::path(std::string(texturePath.C_Str()));
            textureExportPath = sceneContext.GetOutputDirectory() / ".." / textureExportPath;
            textureExportPath.replace_extension("text");

            material.m_albedoMapID = AssetID(textureExportPath.string());
        }

        auto id = AssetID(sceneContext.GetOutputDirectory().string() + "/" + name + ".mtrl");

        AssetArchiveHeader header(id.GetAssetTypeID()); // TODO: Use Material::GetStaticAssetType
        header.AddDependency(material.m_albedoMapID);

        std::vector<std::byte> data;
        auto dataStream = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        material.Serialize(dataStream);

        auto archive = BinaryFileArchive(std::filesystem::path(id.GetAssetPath()), BinaryFileArchive::IOMode::Write);
        archive << header << data;
        // TODO: Other textures
        // TODO: Material properties
    }
};
} // namespace aln::assets::converter
