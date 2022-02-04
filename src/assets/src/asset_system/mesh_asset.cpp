#include "asset_system/mesh_asset.hpp"

#include <json/json.hpp>
#include <lz4.h>

namespace aln::assets
{
using json = nlohmann::json;

// VertexFormat ParseFormat(const char* f)
// {

//     if (strcmp(f, "PNCV_F32") == 0)
//     {
//         return assets::VertexFormat::PNCV_F32;
//     }
//     else if (strcmp(f, "P32N8C8V16") == 0)
//     {
//         return assets::VertexFormat::P32N8C8V16;
//     }
//     else
//     {
//         return assets::VertexFormat::Unknown;
//     }
// }

MeshInfo ReadMeshInfo(AssetFile* file)
{
    MeshInfo info;

    json metadata = json::parse(file->metadata);

    info.vertexBufferSize = metadata["vertex_buffer_size"];
    info.indexBufferSize = metadata["index_buffer_size"];
    info.indexTypeSize = (uint8_t) metadata["index_type_size"];
    info.originalFile = metadata["original_file"];
    info.compressionMode = metadata["compression"];

    std::vector<float> boundsData;
    boundsData.reserve(7);
    boundsData = metadata["bounds"].get<std::vector<float>>();

    info.bounds.origin[0] = boundsData[0];
    info.bounds.origin[1] = boundsData[1];
    info.bounds.origin[2] = boundsData[2];

    info.bounds.radius = boundsData[3];

    info.bounds.extents[0] = boundsData[4];
    info.bounds.extents[1] = boundsData[5];
    info.bounds.extents[2] = boundsData[6];

    // std::string vertexFormat = metadata["vertex_format"];
    // info.vertexFormat = parse_format(vertexFormat.c_str());
    return info;
}

void UnpackMesh(const MeshInfo* info, const std::vector<std::byte>& sourceBuffer, std::byte* vertexBuffer, std::byte* indexBuffer)
{
    // Decompressing into tmp vector.
    // TODO: Skip this step and decompress directly on the buffers
    std::vector<std::byte> decompressedBuffer;
    decompressedBuffer.resize(info->vertexBufferSize + info->indexBufferSize);

    LZ4_decompress_safe(
        reinterpret_cast<const char*>(sourceBuffer.data()),
        reinterpret_cast<char*>(decompressedBuffer.data()),
        static_cast<int>(sourceBuffer.size()),
        static_cast<int>(decompressedBuffer.size()));

    // Copy vertex buffer
    memcpy(vertexBuffer, decompressedBuffer.data(), info->vertexBufferSize);

    // Copy index buffer
    memcpy(indexBuffer, decompressedBuffer.data() + info->vertexBufferSize, info->indexBufferSize);
}

AssetFile PackMesh(MeshInfo* info, char* vertexData, char* indexData)
{
    AssetFile file;
    file.type = EAssetType::Mesh;
    file.version = 1;

    // Pack bounds info
    std::vector<float> boundsData;
    boundsData.resize(7);

    boundsData[0] = info->bounds.origin[0];
    boundsData[1] = info->bounds.origin[1];
    boundsData[2] = info->bounds.origin[2];

    boundsData[3] = info->bounds.radius;

    boundsData[4] = info->bounds.extents[0];
    boundsData[5] = info->bounds.extents[1];
    boundsData[6] = info->bounds.extents[2];

    size_t fullSize = info->vertexBufferSize + info->indexBufferSize;
    std::vector<std::byte> mergedBuffer(fullSize);

    // Copy vertex buffer
    memcpy(mergedBuffer.data(), vertexData, info->vertexBufferSize);

    // Copy index buffer
    memcpy(mergedBuffer.data() + info->vertexBufferSize, indexData, info->indexBufferSize);

    // Find the worst-case compressed size
    size_t maxCompressedSize = LZ4_compressBound(static_cast<int>(fullSize));
    file.binary.resize(maxCompressedSize);

    // Compress buffer and copy it into the file struct
    int compressedSize = LZ4_compress_default(
        reinterpret_cast<char*>(mergedBuffer.data()),
        reinterpret_cast<char*>(file.binary.data()),
        static_cast<int>(mergedBuffer.size()),
        static_cast<int>(maxCompressedSize));

    // Resize back to the actual compressed size
    file.binary.resize(compressedSize);

    json metadata;
    metadata["vertex_buffer_size"] = info->vertexBufferSize;
    metadata["index_buffer_size"] = info->indexBufferSize;
    metadata["index_type_size"] = info->indexTypeSize;
    metadata["bounds"] = boundsData;
    metadata["original_file"] = info->originalFile;
    metadata["compression"] = CompressionMode::LZ4;
    // if (info->vertexFormat == VertexFormat::P32N8C8V16)
    // {
    //     metadata["vertex_format"] = "P32N8C8V16";
    // }
    // else if (info->vertexFormat == VertexFormat::PNCV_F32)
    // {
    //     metadata["vertex_format"] = "PNCV_F32";
    // }

    file.metadata = metadata.dump();

    return file;
}

MeshBounds CalculateBounds(Vertex* vertices, size_t count)
{
    MeshBounds bounds;

    float min[3] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    float max[3] = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

    for (int i = 0; i < count; i++)
    {
        min[0] = std::min(min[0], vertices[i].pos[0]);
        min[1] = std::min(min[1], vertices[i].pos[1]);
        min[2] = std::min(min[2], vertices[i].pos[2]);

        max[0] = std::max(max[0], vertices[i].pos[0]);
        max[1] = std::max(max[1], vertices[i].pos[1]);
        max[2] = std::max(max[2], vertices[i].pos[2]);
    }

    bounds.extents[0] = (max[0] - min[0]) / 2.0f;
    bounds.extents[1] = (max[1] - min[1]) / 2.0f;
    bounds.extents[2] = (max[2] - min[2]) / 2.0f;

    bounds.origin[0] = bounds.extents[0] + min[0];
    bounds.origin[1] = bounds.extents[1] + min[1];
    bounds.origin[2] = bounds.extents[2] + min[2];

    // Go through the vertices again to calculate the exact bounding sphere radius
    float r2 = 0;
    for (int i = 0; i < count; i++)
    {

        float offset[3];
        offset[0] = vertices[i].pos[0] - bounds.origin[0];
        offset[1] = vertices[i].pos[1] - bounds.origin[1];
        offset[2] = vertices[i].pos[2] - bounds.origin[2];

        // Pithagoras
        float distance = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
        r2 = std::max(r2, distance);
    }

    bounds.radius = std::sqrt(r2);

    return bounds;
}
} // namespace aln::assets