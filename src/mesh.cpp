#pragma once

#include <unordered_map>
#include <vector>

#include "components.hpp"
#include "core/buffer.hpp"
#include "core/device.hpp"
#include "utils/files.cpp"
#include "vertex.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// TODO: Complete gltf format
// In glTF, meshes are defined as arrays of primitives. Primitives correspond to the data required for GPU draw calls. Primitives specify one or more attributes, corresponding to the vertex attributes used in the draw calls.
struct Primitive
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t materialIndex;
};

class Mesh : public Component
{
  public:
    // TODO: Move vector/buffer pairs to structs
    // TODO: Before that, is there a reason for us to keep vertices/indices data around ?
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    core::Buffer vertexBuffer;
    core::Buffer indexBuffer;
    core::Buffer uniformBuffer;

    std::vector<Primitive> primitives;

    Mesh() {}

    Mesh(std::shared_ptr<core::Device> device, std::string path, glm::vec3 verticesColor = {1.0f, 1.0f, 1.0f})
    {
        // TODO: Store supported file types as enum
        std::string fileExtension = utils::getFileExtension(path);
        if (fileExtension.compare("") == 0)
        {
            throw std::runtime_error("No file extension found in: " + path);
        }
        else if (fileExtension.compare(".obj"))
        {
            loadFromObj(path, verticesColor);
        }
        else
        {
            throw std::runtime_error("File extension " + fileExtension + " is not supported.");
        }

        createDataBuffers(device);
        createUniformBuffer(device);
    }

    void loadFromObj(std::string path, glm::vec3 verticesColor = {1.0f, 1.0f, 1.0f})
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                // TODO: Build primitives on the go
                Vertex vertex = {};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

                if (index.texcoord_index >= 0)
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip the vertical component (.obj files assume 0 means bottom of image, we assume it means top)
                    };
                }

                vertex.color = verticesColor;

                if (index.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    this->vertices.push_back(vertex);
                }

                this->indices.push_back(uniqueVertices[vertex]);
            }

            // TODO: Each shape is a primitive
        }

        this->primitives.push_back({0, (uint32_t) this->indices.size(), 0});
    }

    void createDataBuffers(std::shared_ptr<core::Device> device)
    {
        // Create vertex buffer
        core::Buffer vertexStagingBuffer(device, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertices);
        vertexBuffer = core::Buffer(device, vertexStagingBuffer.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        // Create index buffer
        core::Buffer indexStagingBuffer(device, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indices);
        indexBuffer = core::Buffer(device, indexStagingBuffer.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        device->commandpools.transfer.execute([&](vk::CommandBuffer cb) {
            vertexStagingBuffer.copyTo(cb, vertexBuffer);
            indexStagingBuffer.copyTo(cb, indexBuffer);
        });
    }

    void createUniformBuffer(std::shared_ptr<core::Device> device)
    {
        uniformBuffer = core::Buffer(device, sizeof(core::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    void updateUniformBuffers(core::UniformBufferObject ubo)
    {
        uniformBuffer.map(0, sizeof(ubo));
        uniformBuffer.copy(&ubo, sizeof(ubo));
        uniformBuffer.unmap();
    }

    void revertNormals()
    {
        for (Vertex v : vertices)
        {
            v.normal = -v.normal;
        }
    }
};