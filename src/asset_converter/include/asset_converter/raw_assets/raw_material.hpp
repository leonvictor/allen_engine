#pragma once

#include <assert.h>

#include <assets/asset_archive_header.hpp>
#include <common/serialization/binary_archive.hpp>

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"
#include "raw_texture.hpp"

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
    static AssetID ReadMaterial(const AssimpSceneContext& sceneContext, const aiMaterial* pMaterial)
    {
        RawMaterial material;
        auto name = std::string(pMaterial->GetName().C_Str());

        // TODO: What are those empty materials ?
        if (name.empty())
        {
            return AssetID();
        }

        // Diffuse
        auto textureCount = pMaterial->GetTextureCount(aiTextureType_DIFFUSE);
        assert(textureCount <= 1); // We only support one texture for now
        for (auto textureIndex = 0; textureIndex < textureCount; ++textureIndex)
        {
            // TODO: Use StaticAssetType
            std::filesystem::path textureExportPath = sceneContext.GetOutputDirectory() / (name + "_Albedo" + ".text");

            aiString texturePath;
            pMaterial->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath, nullptr, nullptr, nullptr, nullptr, nullptr);

            AssetID textureID;
            const auto pEmbeddedTexture = sceneContext.GetScene()->GetEmbeddedTexture(texturePath.C_Str());
            if (pEmbeddedTexture != nullptr)
            {
                textureID = AssimpTextureReader::ReadTexture(sceneContext, pEmbeddedTexture, textureExportPath);
            }
            else
            {
                auto textureFilePath = sceneContext.GetSourceFile();
                textureFilePath.replace_filename(texturePath.C_Str());
                textureID = FileTextureReader::ReadTexture(textureFilePath, textureExportPath);
            }
            material.m_albedoMapID = textureID;
        }

        auto exportPath = sceneContext.GetOutputDirectory() / name;
        exportPath.replace_extension("mtrl");
        auto id = AssetID(exportPath.string());

        AssetArchiveHeader header(id.GetAssetTypeID()); // TODO: Use Material::GetStaticAssetType
        header.AddDependency(material.m_albedoMapID);

        Vector<std::byte> data;
        auto dataStream = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        material.Serialize(dataStream);

        auto archive = BinaryFileArchive(std::filesystem::path(id.GetAssetPath()), BinaryFileArchive::IOMode::Write);
        archive << header << data;
        // TODO: Other textures
        // TODO: Material properties

        return id;
    }
};
} // namespace aln::assets::converter
