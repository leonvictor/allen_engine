#pragma once

#include <assets/asset_archive_header.hpp>
#include <assets/asset_id.hpp>
#include <common/serialization/binary_archive.hpp>
#include <common/vertex.hpp>

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"
#include "raw_skeleton.hpp"

#include <assert.h>
#include <filesystem>
#include <vector>

namespace aln::assets::converter
{
class RawStaticMesh : public IRawAsset
{
    friend class AssimpMeshReader;

    std::vector<uint32_t> m_indices;
    std::vector<Vertex> m_vertices;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        archive << m_indices;

        // Vertices need to be extracted in a std::vector<std::byte> format
        size_t byteSize = m_vertices.size() * sizeof(SkinnedVertex);
        archive << byteSize;
        archive.Write(m_vertices.data(), byteSize);
    }
};

class RawSkeletalMesh : public IRawAsset
{
    friend class AssimpMeshReader;

    std::vector<uint32_t> m_indices;
    std::vector<SkinnedVertex> m_vertices;
    std::vector<Transform> m_inverseBindPose;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        archive << m_indices;

        // Vertices need to be extracted in a std::vector<std::byte> format
        size_t byteSize = m_vertices.size() * sizeof(SkinnedVertex);
        archive << byteSize;
        archive.Write(m_vertices.data(), byteSize);

        archive << m_inverseBindPose;
    }

    static void AddBoneData(SkinnedVertex& vertex, uint32_t boneIndex, float weight)
    {
        /// @note Max influenced vertices is 4
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
};

struct AssimpMeshReader
{
    template <typename T>
    static void ReadMeshData(std::vector<T>& vertices, std::vector<uint32_t>& indices, const aiMesh* pMesh, const AssimpSceneContext& context)
    {
        for (int vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
        {
            auto& vertex = vertices.emplace_back();

            vertex.pos = context.ToGLM(pMesh->mVertices[vertexIndex]);
            vertex.normal = context.ToGLM(pMesh->mNormals[vertexIndex]);

            if (pMesh->GetNumUVChannels() >= 1)
            {
                vertex.texCoord[0] = pMesh->mTextureCoords[0][vertexIndex].x;
                vertex.texCoord[1] = pMesh->mTextureCoords[0][vertexIndex].y;
            }

            if (pMesh->HasVertexColors(0))
            {
                vertex.color = context.ToVec3(*pMesh->mColors[0]);
            }
            else
            {
                vertex.color = {1, 1, 1};
            }
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
            //     int v0 = vertex.m_indices[faceIndex * 3 + 0];
            //     int v1 = vertex.m_indices[faceIndex * 3 + 1];
            //     int v2 = vertex.m_indices[faceIndex * 3 + 2];

            //     glm::vec3 normal = glm::normalize(glm::cross(
            //         vertices[v2].pos - vertices[v0].pos,
            //         vertices[v1].pos - vertices[v0].pos));

            //     memcpy(&vertices[v0].normal, &normal, sizeof(float) * 3);
            //     memcpy(&vertices[v1].normal, &normal, sizeof(float) * 3);
            //     memcpy(&vertices[v2].normal, &normal, sizeof(float) * 3);
            // }
        }
    }

    static void ReadMesh(AssimpSceneContext& context, const aiMesh* pMesh)
    {
        // TODO: Output naming
        std::string meshName = std::string(pMesh->mName.C_Str());
        std::filesystem::path meshPath = context.GetOutputDirectory() / (meshName);

        if (pMesh->HasBones())
        {
            RawSkeletalMesh mesh;

            ReadMeshData(mesh.m_vertices, mesh.m_indices, pMesh, context);

            // Read Skeleton asset
            const auto pSkeleton = AssimpSkeletonReader::ReadSkeleton(context, pMesh);
            assert((pSkeleton->GetBoneCount() - 1) == pMesh->mNumBones);

            mesh.m_inverseBindPose.resize(pSkeleton->GetBoneCount(), Transform::Identity);
            for (size_t meshBoneIndex = 0; meshBoneIndex < pMesh->mNumBones; ++meshBoneIndex)
            {
                auto pBone = pMesh->mBones[meshBoneIndex];
                auto skeletonBoneIndex = pSkeleton->GetBoneIndex(std::string(pBone->mName.C_Str()));
                assert(skeletonBoneIndex != InvalidIndex);

                // Save the inverse bind pose matrix
                mesh.m_inverseBindPose[skeletonBoneIndex] = context.DecomposeMatrix(pBone->mOffsetMatrix);

                // Update influenced vertices
                for (size_t weightIndex = 0; weightIndex < pBone->mNumWeights; ++weightIndex)
                {
                    const auto& weight = pBone->mWeights[weightIndex];
                    auto& influencedVertex = mesh.m_vertices[weight.mVertexId];
                    RawSkeletalMesh::AddBoneData(influencedVertex, skeletonBoneIndex, weight.mWeight);
                }
            }

            // Save skeletal mesh asset
            meshPath += ".smsh";

            AssetArchiveHeader header("smsh"); // TODO: Use SkeletalMesh::GetStaticAssetType();

            std::vector<std::byte> data;
            BinaryMemoryArchive dataStream(data, IBinaryArchive::IOMode::Write);

            mesh.Serialize(dataStream);

            // TODO: Compress
            // TODO: Calculate AABB

            // TODO: Generate AssetID;
            auto assetID = context.GetOutputDirectory() / (meshName + ".smsh");
            auto archive = BinaryFileArchive(assetID.string(), IBinaryArchive::IOMode::Write);

            archive << header << data;
        }
        else
        { // Static Mesh

            RawStaticMesh mesh;
            ReadMeshData(mesh.m_vertices, mesh.m_indices, pMesh, context);

            AssetArchiveHeader header("mesh"); // TODO: Use StaticMesh::GetStaticAssetType();

            std::vector<std::byte> data;
            BinaryMemoryArchive dataStream(data, IBinaryArchive::IOMode::Write);

            mesh.Serialize(dataStream);

            // TODO: Compress
            // TODO: AABB

            // TODO: Generate AssetID;
            auto assetID = context.GetOutputDirectory() / (meshName + ".mesh");
            auto archive = BinaryFileArchive(assetID.string(), IBinaryArchive::IOMode::Write);

            archive << header << data;
        }
    }
};
} // namespace aln::assets::converter