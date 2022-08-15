#pragma once

#include <assimp/scene.h>

#include <glm/vec3.hpp>

#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/material_asset.hpp>
#include <assets/asset_system/mesh_asset.hpp>

#include "assimp_animation.hpp"
#include "assimp_material.hpp"
#include "assimp_scene_context.hpp"

#include <filesystem>
#include <iostream>

namespace aln::assets::converter
{

// TODO: Only in cpp
namespace fs = std::filesystem;
/// @brief TODO
class AssetConverter
{
  private:
    AssimpSceneContext m_sceneContext;

    AssetConverter(std::filesystem::path inputFile, std::filesystem::path outputDirectory, int postProcessFlags)
        : m_sceneContext(inputFile, outputDirectory)
    {
        fs::create_directory(outputDirectory);

        ExtractMaterials();
        // ExtractMeshes();
        ExtractNodes();

        for (auto animIndex = 0; animIndex < m_sceneContext.GetScene()->mNumAnimations; ++animIndex)
        {
            auto pAnimation = m_sceneContext.GetScene()->mAnimations[animIndex];
            const AssimpSkeleton* pSkeleton = AssimpSkeletonReader::ReadSkeleton(m_sceneContext, pAnimation);
            AssimpAnimationReader::ReadAnimation(m_sceneContext, pAnimation, pSkeleton);
        }
    }

    fs::path GetPathRelativeToOutput(const fs::path& path) const
    {
        return path.lexically_proximate(m_sceneContext.GetOutputDirectory());
    }

    /// @brief Extract materials from a assimp scene and save them in asset binary format
    void ExtractMaterials()
    {
        if (m_sceneContext.GetScene()->HasMaterials())
        {
            for (auto materialIndex = 0; materialIndex < m_sceneContext.GetScene()->mNumMaterials; ++materialIndex)
            {
                auto pMaterial = m_sceneContext.GetScene()->mMaterials[materialIndex];
                AssimpMaterialReader::ReadMaterial(m_sceneContext, pMaterial);
            }
        }
    }

    fs::path GetMaterialAssetPath(uint32_t materialIndex)
    {
        auto pMaterial = m_sceneContext.GetScene()->mMaterials[materialIndex];
        std::string materialName = std::string(pMaterial->GetName().C_Str()) + "_" + std::to_string(materialIndex);
        fs::path materialPath = m_sceneContext.GetOutputDirectory() / (materialName + ".mat");
        return materialPath;
    }

    fs::path GetMeshAssetPath(uint32_t meshIndex)
    {
        auto pMesh = m_sceneContext.GetScene()->mMeshes[meshIndex];
        std::string meshName = std::string(pMesh->mName.C_Str()) + "_" + std::to_string(meshIndex);
        fs::path meshPath = m_sceneContext.GetOutputDirectory() / (meshName + ".mesh");
        return meshPath;
    }

    // -----------------
    // Mesh stuff. @todo: Move to AssimpMesh like the rest, factorize static/skeletal
    // Do that when we use actual serialization
    // -----------------

    /// @brief Process a mesh in assimp format and store processed data in vertex/index buffers
    void ProcessStaticMesh(const aiMesh* pMesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        vertices.reserve(pMesh->mNumVertices);
        for (int vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
        {
            Vertex vert;
            vert.pos = {
                pMesh->mVertices[vertexIndex].x,
                pMesh->mVertices[vertexIndex].y,
                pMesh->mVertices[vertexIndex].z};

            vert.normal[0] = pMesh->mNormals[vertexIndex].x;
            vert.normal[1] = pMesh->mNormals[vertexIndex].y;
            vert.normal[2] = pMesh->mNormals[vertexIndex].z;

            if (pMesh->GetNumUVChannels() >= 1)
            {
                vert.texCoord[0] = pMesh->mTextureCoords[0][vertexIndex].x;
                vert.texCoord[1] = pMesh->mTextureCoords[0][vertexIndex].y;
            }
            else
            {
                vert.texCoord = {0, 0};
            }

            if (pMesh->HasVertexColors(0))
            {
                vert.color[0] = pMesh->mColors[0][vertexIndex].r;
                vert.color[1] = pMesh->mColors[0][vertexIndex].g;
                vert.color[2] = pMesh->mColors[0][vertexIndex].b;
            }
            else
            {
                vert.color = {1, 1, 1};
            }

            vertices.push_back(vert);
        }

        indices.reserve(pMesh->mNumFaces * 3);
        for (int faceIndex = 0; faceIndex < pMesh->mNumFaces; faceIndex++)
        {
            auto& faceIndices = pMesh->mFaces[faceIndex].mIndices;
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[2]);

            // TODO: test this
            // assimp fbx creates bad normals, just regen them
            if (true)
            {
                int v0 = indices[faceIndex * 3 + 0];
                int v1 = indices[faceIndex * 3 + 1];
                int v2 = indices[faceIndex * 3 + 2];

                glm::vec3 normal = glm::normalize(glm::cross(
                    vertices[v2].pos - vertices[v0].pos,
                    vertices[v1].pos - vertices[v0].pos));

                memcpy(&vertices[v0].normal, &normal, sizeof(float) * 3);
                memcpy(&vertices[v1].normal, &normal, sizeof(float) * 3);
                memcpy(&vertices[v2].normal, &normal, sizeof(float) * 3);
            }
        }
    }

    /// @brief Process a mesh in assimp format and store processed data in vertex/index buffers
    void ProcessSkeletalMesh(const aiMesh* pMesh, std::vector<SkinnedVertex>& vertices, std::vector<uint32_t>& indices)
    {
        vertices.reserve(pMesh->mNumVertices);
        for (int vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
        {
            SkinnedVertex vert;

            auto pos = m_sceneContext.ToGLM(pMesh->mVertices[vertexIndex]);
            // vert.pos = m_sceneContext.RevertSceneTransform(pos);
            vert.pos = pos;

            auto normal = m_sceneContext.ToGLM(pMesh->mNormals[vertexIndex]);
            // vert.normal = m_sceneContext.RevertSceneTransform(normal);
            vert.normal = normal;

            if (pMesh->GetNumUVChannels() >= 1)
            {
                vert.texCoord[0] = pMesh->mTextureCoords[0][vertexIndex].x;
                vert.texCoord[1] = pMesh->mTextureCoords[0][vertexIndex].y;
            }
            else
            {
                vert.texCoord[0] = 0;
                vert.texCoord[1] = 0;
            }

            if (pMesh->HasVertexColors(0))
            {
                vert.color[0] = pMesh->mColors[0][vertexIndex].r;
                vert.color[1] = pMesh->mColors[0][vertexIndex].g;
                vert.color[2] = pMesh->mColors[0][vertexIndex].b;
            }
            else
            {
                vert.color[0] = 1;
                vert.color[1] = 1;
                vert.color[2] = 1;
            }

            vert.boneIndices = {0, 0, 0, 0};
            vert.weights = {0, 0, 0, 0};
            vertices.push_back(vert);
        }

        indices.reserve(pMesh->mNumFaces * 3);
        for (int faceIndex = 0; faceIndex < pMesh->mNumFaces; faceIndex++)
        {
            auto& faceIndices = pMesh->mFaces[faceIndex].mIndices;
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[2]);

            // TODO: test this
            // assimp fbx creates bad normals, just regen them
            // if (true)
            // {
            //     int v0 = indices[faceIndex * 3 + 0];
            //     int v1 = indices[faceIndex * 3 + 1];
            //     int v2 = indices[faceIndex * 3 + 2];

            //     glm::vec3 normal = glm::normalize(glm::cross(
            //         vertices[v2].pos - vertices[v0].pos,
            //         vertices[v1].pos - vertices[v0].pos));

            //     memcpy(&vertices[v0].normal, &normal, sizeof(float) * 3);
            //     memcpy(&vertices[v1].normal, &normal, sizeof(float) * 3);
            //     memcpy(&vertices[v2].normal, &normal, sizeof(float) * 3);
            // }
        }
    }

    void AddBoneData(SkinnedVertex& vertex, uint32_t boneIndex, float weight)
    {
        for (uint8_t idx = 0; idx < 4; ++idx)
        {
            if (vertex.weights[idx] == 0)
            {
                vertex.boneIndices[idx] = boneIndex;
                vertex.weights[idx] = weight;
                return;
            }
        }

        // Too many weights for a single vertex
        assert(false);
    }

    void ExtractMesh(aiMesh* pMesh)
    {
        Transform transformBuffer;
        AssetFile file;
        std::vector<uint32_t> indices;

        // TODO: Output naming
        // std::string meshName = std::string(pMesh->mName.C_Str()) + "_" + std::to_string(meshIndex);
        std::string meshName = std::string(pMesh->mName.C_Str());
        fs::path meshPath = m_sceneContext.GetOutputDirectory() / (meshName);

        if (pMesh->HasBones())
        {
            // Read Skeleton asset
            const auto* pSkeleton = AssimpSkeletonReader::ReadSkeleton(m_sceneContext, pMesh);

            std::vector<SkinnedVertex> vertices;
            ProcessSkeletalMesh(pMesh, vertices, indices);

            std::vector<Transform> inverseBindPose;
            inverseBindPose.resize(pSkeleton->GetBoneCount(), Transform::Identity);

            for (size_t meshBoneIndex = 0; meshBoneIndex < pMesh->mNumBones; ++meshBoneIndex)
            {
                auto pBone = pMesh->mBones[meshBoneIndex];
                auto skeletonBoneIndex = pSkeleton->GetBoneIndex(std::string(pBone->mName.C_Str()));
                assert(skeletonBoneIndex != InvalidIndex);

                // Save the inverse bind pose matrix
                inverseBindPose[skeletonBoneIndex] = m_sceneContext.DecomposeMatrix(pBone->mOffsetMatrix);

                // Update influenced vertices
                for (size_t weightIndex = 0; weightIndex < pBone->mNumWeights; ++weightIndex)
                {
                    const auto& weight = pBone->mWeights[weightIndex];
                    auto& influencedVertex = vertices[weight.mVertexId];
                    AddBoneData(influencedVertex, skeletonBoneIndex, weight.mWeight);
                }
            }

            // Save skeletal mesh asset
            meshPath += ".smsh";

            SkeletalMeshInfo meshInfo;
            meshInfo.originalFile = m_sceneContext.GetSourceFile().string();
            meshInfo.indexTypeSize = sizeof(uint32_t);
            meshInfo.vertexBufferSize = vertices.size() * sizeof(SkinnedVertex);
            meshInfo.indexBufferSize = indices.size() * sizeof(uint32_t);
            meshInfo.bounds = SkeletalMeshConverter::CalculateBounds(vertices.data(), vertices.size());
            meshInfo.inverseBindPoseSize = inverseBindPose.size() * sizeof(Transform);
            file = SkeletalMeshConverter::Pack(&meshInfo, (char*) vertices.data(), (char*) indices.data(), (char*) inverseBindPose.data());
        }
        else
        { // Static Mesh
            std::vector<Vertex> vertices;
            ProcessStaticMesh(pMesh, vertices, indices);

            StaticMeshInfo meshInfo;
            meshInfo.originalFile = m_sceneContext.GetSourceFile().string();
            meshInfo.indexTypeSize = sizeof(uint32_t);
            meshInfo.vertexBufferSize = vertices.size() * sizeof(Vertex);
            meshInfo.indexBufferSize = indices.size() * sizeof(uint32_t);
            meshInfo.bounds = StaticMeshConverter::CalculateBounds(vertices.data(), vertices.size());

            // TODO: Use StaticMesh::GetStaticTypeID();
            meshPath += ".mesh";

            file = StaticMeshConverter::Pack(&meshInfo, (char*) vertices.data(), (char*) indices.data());
        }

        // Save to disk
        SaveBinaryFile(meshPath.string(), file);
    }

    void ExtractNodes()
    {
        uint64_t lastNode = 0;
        uint8_t depth = 0;
        uint32_t nodeCount = 0;

        std::function<void(aiNode*, uint64_t, const aiMatrix4x4&)> processNode = [&](aiNode* pNode, uint64_t parentID, const aiMatrix4x4& parentGlobalTransform)
        {
            nodeCount++;

            uint64_t nodeindex = lastNode;
            lastNode++;

            std::string indentString;
            for (int i = 0; i < depth; i++)
            {
                indentString += "  ";
            }

            auto t = m_sceneContext.DecomposeMatrix(pNode->mTransformation);
            std::cout << indentString << std::string(pNode->mName.C_Str()) << std::endl;

            auto nodeLocalTransform = pNode->mTransformation;
            auto nodeGlobalTransform = nodeLocalTransform * parentGlobalTransform;

            // Extract the node mesh/material pair
            /// @note Assimp does not handle multiple material per mesh and will instead split
            /// meshes with multiple material
            if (pNode->mNumMeshes > 0)
            {
                for (size_t nodeMeshIndex = 0; nodeMeshIndex < pNode->mNumMeshes; ++nodeMeshIndex)
                {
                    auto pMesh = m_sceneContext.GetScene()->mMeshes[pNode->mMeshes[nodeMeshIndex]];
                    ExtractMesh(pMesh);
                }
            }

            depth++;
            for (size_t ch = 0; ch < pNode->mNumChildren; ++ch)
            {
                processNode(pNode->mChildren[ch], nodeindex, nodeGlobalTransform);
            }
            depth--;
        };

        processNode(m_sceneContext.GetScene()->mRootNode, 0, m_sceneContext.GetScene()->mRootNode->mTransformation);
    }

  public:
    static void Convert(std::filesystem::path inputFile, std::filesystem::path outputDirectory, int postProcessFlags)
    {
        auto converter = AssetConverter(inputFile, outputDirectory, postProcessFlags);
    }
};

} // namespace aln::assets::converter