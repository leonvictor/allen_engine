#pragma once

#include <assimp/scene.h>

#include "assimp_scene_context.hpp"
#include "raw_assets/raw_animation.hpp"
#include "raw_assets/raw_material.hpp"
#include "raw_assets/raw_mesh.hpp"
#include "raw_assets/raw_skeleton.hpp"

#include <filesystem>

namespace aln::assets::converter
{

/// @brief TODO
class AssetConverter
{
  private:
    AssimpSceneContext m_sceneContext;

    AssetConverter(std::filesystem::path inputFile, std::filesystem::path outputDirectory, int postProcessFlags)
        : m_sceneContext(inputFile, outputDirectory)
    {
        std::filesystem::create_directory(outputDirectory);

        ReadMaterials();
        ReadMeshes();
        ReadAnimations();
    }

    void ReadAnimations()
    {
        const auto pScene = m_sceneContext.GetScene();
        if (pScene->HasAnimations())
        {
            for (auto animIndex = 0; animIndex < pScene->mNumAnimations; ++animIndex)
            {
                auto pAnimation = pScene->mAnimations[animIndex];
                const auto pSkeleton = AssimpSkeletonReader::ReadSkeleton(m_sceneContext, pAnimation);
                AssimpAnimationReader::ReadAnimation(m_sceneContext, pAnimation, pSkeleton);
            }
        }
    }

    void ReadMeshes()
    {
        const auto& pScene = m_sceneContext.GetScene();
        if (pScene->HasMeshes())
        {
            for (auto meshIndex = 0; meshIndex < pScene->mNumMeshes; ++meshIndex)
            {
                auto pMesh = pScene->mMeshes[meshIndex];
                AssimpMeshReader::ReadMesh(m_sceneContext, pMesh);
            }
        }
    }

    void ReadMaterials()
    {
        const auto pScene = m_sceneContext.GetScene();
        if (pScene->HasMaterials())
        {
            for (auto materialIndex = 0; materialIndex < pScene->mNumMaterials; ++materialIndex)
            {
                auto pMaterial = pScene->mMaterials[materialIndex];
                auto id = AssimpMaterialReader::ReadMaterial(m_sceneContext, pMaterial);
                m_sceneContext.m_materials.push_back(id);
            }
        }
    }

  public:
    static void Convert(std::filesystem::path inputFile, std::filesystem::path outputDirectory, int postProcessFlags)
    {
        auto converter = AssetConverter(inputFile, outputDirectory, postProcessFlags);
    }
};

} // namespace aln::assets::converter