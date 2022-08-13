#pragma once

#include "asset_system.hpp"

#include <common/vertex.hpp>

namespace aln::assets
{

struct MeshBounds
{
    float origin[3];
    float radius;
    float extents[3];
};

struct StaticMeshInfo
{
    uint64_t vertexBufferSize;
    uint64_t indexBufferSize;
    MeshBounds bounds;
    // VertexFormat vertexFormat; // TODO: that's a good idea

    uint8_t indexTypeSize; // In-memory size of an index (in bytes)
    CompressionMode compressionMode;
    std::string originalFile;
};

struct StaticMeshConverter
{
    /// @brief Read a mesh's AssetFile and populate its StaticMeshInfo
    static StaticMeshInfo ReadInfo(AssetFile* file);
    static void Unpack(const StaticMeshInfo* info, const std::vector<std::byte>& sourcebuffer, std::byte* vertexBuffer, std::byte* indexBuffer);
    static AssetFile Pack(StaticMeshInfo* info, char* vertexData, char* indexData);
    static MeshBounds CalculateBounds(Vertex* vertices, size_t count);
};

struct SkeletalMeshInfo
{
    uint64_t vertexBufferSize;
    uint64_t indexBufferSize;
    uint64_t inverseBindPoseSize;

    MeshBounds bounds;

    uint8_t indexTypeSize; // In-memory size of an index (in bytes)
    CompressionMode compressionMode;
    std::string originalFile;
};

struct SkeletalMeshConverter
{
    static SkeletalMeshInfo ReadInfo(AssetFile* file);
    static void Unpack(const SkeletalMeshInfo* info, const std::vector<std::byte>& sourcebuffer, std::byte* vertexBuffer, std::byte* indexBuffer, std::byte* bindPose);
    static AssetFile Pack(SkeletalMeshInfo* info, char* vertexData, char* indexData, char* bindPose);
    static MeshBounds CalculateBounds(SkinnedVertex* vertices, size_t count);
};
} // namespace aln::assets