#pragma once

#include "asset_system.hpp"

#include <common/vertex.hpp>

namespace aln::assets
{

// struct Vertex_f32_PNCV
// {

//     float position[3];
//     float normal[3];
//     float color[3];
//     float uv[2];
// };
// struct Vertex_P32N8C8V16
// {

//     float position[3];
//     uint8_t normal[3];
//     uint8_t color[3];
//     float uv[2];
// };

// enum class VertexFormat : uint32_t
// {
//     Unknown = 0,
//     PNCV_F32,  //everything at 32 bits
//     P32N8C8V16 //position at 32 bits, normal at 8 bits, color at 8 bits, uvs at 16 bits float
// };

struct MeshBounds
{
    float origin[3];
    float radius;
    float extents[3];
};

struct MeshInfo
{
    uint64_t vertexBufferSize;
    uint64_t indexBufferSize;
    MeshBounds bounds;
    // VertexFormat vertexFormat; // TODO: that's a good idea

    uint8_t indexTypeSize; // In-memory size of an index (in bytes)
    CompressionMode compressionMode;
    std::string originalFile;
};

MeshInfo ReadMeshInfo(AssetFile* file);

void UnpackMesh(const MeshInfo* info, const std::vector<std::byte>& sourcebuffer, std::byte* vertexBuffer, std::byte* indexBuffer);
AssetFile PackMesh(MeshInfo* info, char* vertexData, char* indexData);

MeshBounds CalculateBounds(Vertex* vertices, size_t count);
} // namespace aln::assets