#pragma once

#include <assets/asset_id.hpp>

#include "assimp_scene_context.hpp"

#include <assert.h>

namespace aln::assets::converter
{
/// @todo : Should be approx. the same as the runtime Material class
/// @todo : Better conversion to disk format
class AssimpMaterial
{
    friend class AssimpMaterialReader;

  private:
    AssetID m_id;
    std::string m_name;

    AssetID m_albedoMapID;

  public:
    const std::string& GetName() { return m_name; }

    bool SaveToBinary(const AssimpSceneContext& sceneContext)
    {
        assert(!m_id.IsValid());
        m_id = AssetID(sceneContext.GetOutputDirectory().string() + "/" + m_name + ".mtrl");

        // TODO: Replace with proper serialization
        MaterialInfo materialInfo;
        materialInfo.m_name = m_name;
        materialInfo.m_albedoMapID = m_albedoMapID;

        auto file = PackMaterial(&materialInfo);
        file.m_dependencies.push_back(m_albedoMapID);

        return SaveBinaryFile(m_id.GetAssetPath(), file);
    }
};

class AssimpMaterialReader
{
  public:
    static void ReadMaterial(const AssimpSceneContext& sceneContext, const aiMaterial* pMaterial)
    {
        AssimpMaterial material;
        material.m_name = std::string(pMaterial->GetName().C_Str());

        // TODO: What are those empty materials ?
        if (material.m_name.empty())
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

        material.SaveToBinary(sceneContext);

        // TODO: Other textures
        // TODO: Material properties
    }
};
} // namespace aln::assets::converter