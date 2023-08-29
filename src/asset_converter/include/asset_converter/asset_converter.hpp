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
    Assimp::Importer m_importer;
    AssimpSceneContext m_sceneContext;

    static constexpr int AssimpPostProcessFlags =
        aiProcess_GenNormals |
        // aiProcess_JoinIdenticalVertices | // TMP: aiProcess_JoinIndenticalVertices provokes extremely long processing times in debug mode...
        aiProcess_FlipUVs |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_PopulateArmatureData;

  public:
    AssetConverter(std::filesystem::path& outputDirectory)
    {
        std::filesystem::create_directory(outputDirectory);
        m_sceneContext.m_outputDirectoryPath = outputDirectory;
        m_sceneContext.m_outputDirectoryPath.make_preferred();

        m_importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    }

    void RecursivePrint(const aiNode* pNode, int depthLevel=0)
    {
        for (int i = 0; i < depthLevel; ++i)
        {
            std::cout << " ";
        }

        aiVector3D pos, scale, rot;
        pNode->mTransformation.Decompose(scale, rot, pos);
        std::cout << pNode->mName.C_Str() << " (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        
        depthLevel++;
        for (auto i = 0; i < pNode->mNumChildren; ++i)
        {
            RecursivePrint(pNode->mChildren[i], depthLevel);
        }
    }

    void ReadFile(std::filesystem::path inputFile)
    {
        m_sceneContext.m_pScene = m_importer.ReadFile(inputFile.string(), AssimpPostProcessFlags);
        m_sceneContext.m_sourceFilePath = inputFile;
        m_sceneContext.m_sourceFilePath.make_preferred();

        if (m_sceneContext.m_pScene == nullptr)
        {
            // TODO: Handle error
            auto errorString = m_importer.GetErrorString();
            assert(false);
        }

        //RecursivePrint(m_sceneContext.GetRootNode());

        m_sceneContext.m_inverseSceneTransform = m_sceneContext.DecomposeMatrix(m_sceneContext.m_pScene->mRootNode->mTransformation).GetInverse();

        ReadMaterials();
        ReadMeshes();
        ReadAnimations();
    }

  private:
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
};

} // namespace aln::assets::converter