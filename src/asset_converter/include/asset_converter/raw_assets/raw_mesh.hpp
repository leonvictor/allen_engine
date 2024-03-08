#pragma once

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"
#include "raw_skeleton.hpp"

#include <assets/asset_archive_header.hpp>
#include <assets/asset_id.hpp>
#include <common/containers/vector.hpp>
#include <common/serialization/binary_archive.hpp>
#include <common/vertex.hpp>

#include <assert.h>
#include <filesystem>

namespace aln::assets::converter
{
class RawStaticMesh : public IRawAsset
{
    friend class AssimpMeshReader;

    Vector<uint32_t> m_indices;
    Vector<Vertex> m_vertices;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        archive << m_indices;

        // Vertices need to be extracted in a Vector<std::byte> format
        size_t byteSize = m_vertices.size() * sizeof(Vertex);
        archive << byteSize;
        archive.Write(m_vertices.data(), byteSize);
    }
};

class RawSkeletalMesh : public IRawAsset
{
    friend class AssimpMeshReader;

    Vector<uint32_t> m_indices;
    Vector<SkinnedVertex> m_vertices;
    Vector<Transform> m_inverseBindPose;

    RawSkeleton m_skeleton;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        archive << m_indices;

        // Vertices need to be extracted in a Vector<std::byte> format
        size_t byteSize = m_vertices.size() * sizeof(SkinnedVertex);
        archive << byteSize;
        archive.Write(m_vertices.data(), byteSize);

        archive << m_skeleton.m_boneNames;
        archive << m_skeleton.m_parentBoneIndices;
        archive << m_inverseBindPose;
    }

    static void AddBoneData(SkinnedVertex& vertex, uint32_t boneIndex, float weight)
    {
        /// @note Max influenced vertices is 4.
        // TODO: For now if more weights are detected only the most influent one are kept.
        uint8_t minIndex = 5;
        float minWeight = 1;
        for (uint8_t idx = 0; idx < 4; ++idx)
        {
            if (vertex.weights[idx] == 0)
            {
                vertex.boneIndices[idx] = boneIndex;
                vertex.weights[idx] = weight;
                return;
            }
            else if (weight > vertex.weights[idx] && minWeight > vertex.weights[idx])
            {
                minIndex = idx;
                minWeight = weight;
            }
        }

        if (minIndex < 5)
        {
            vertex.weights[minIndex] = weight;
            vertex.boneIndices[minIndex] = boneIndex;
        }

        // Too many weights for a single vertex
        // assert(false);
    }
};

struct AssimpMeshReader
{
    template <typename T>
    static void ReadMeshData(Vector<T>& vertices, Vector<uint32_t>& indices, const aiMesh* pMesh, const AssimpSceneContext& context)
    {
        for (int vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
        {
            auto& vertex = vertices.emplace_back();

            vertex.pos = context.ToVec3(pMesh->mVertices[vertexIndex]);
            vertex.normal = context.ToVec3(pMesh->mNormals[vertexIndex]);

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

            //     Vec3 normal = glm::normalize(glm::cross(
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
            AssimpSkeletonReader::ReadSkeleton(context, pMesh, &mesh.m_skeleton);

            // TODO: This should be done directly in the skeleton reader method
            assert((mesh.m_skeleton.GetBonesCount()) == pMesh->mNumBones);

            mesh.m_inverseBindPose.resize(mesh.m_skeleton.GetBonesCount(), Transform::Identity);
            for (size_t meshBoneIndex = 0; meshBoneIndex < pMesh->mNumBones; ++meshBoneIndex)
            {
                const auto pBone = pMesh->mBones[meshBoneIndex];
                const auto skeletonBoneIndex = mesh.m_skeleton.GetBoneIndex(std::string(pBone->mName.C_Str()));
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
            AssetArchiveHeader header("smsh"); // TODO: Use SkeletalMesh::GetStaticAssetType();
            header.AddDependency(context.GetMaterial(pMesh->mMaterialIndex));

            Vector<std::byte> data;
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
            header.AddDependency(context.GetMaterial(pMesh->mMaterialIndex));

            Vector<std::byte> data;
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