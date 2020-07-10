#pragma once

#include <unordered_map>
#include <vector>

#include "core/buffer.hpp"
#include "core/context.hpp"
#include "core/device.hpp"
#include "vertex.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

class Mesh
{
  public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    core::Buffer vertexBuffer;
    core::Buffer indexBuffer;
    core::Buffer uniformBuffer;

    Mesh() {}

    std::string getFileExtension(const std::string& FileName)
    {
        if (FileName.find_last_of(".") != std::string::npos)
            return FileName.substr(FileName.find_last_of(".") + 1);
        return "";
    }

    Mesh(std::shared_ptr<core::Device> device, std::string path, glm::vec3 verticesColor = {1.0f, 1.0f, 1.0f})
    {
        std::string fileExtension = getFileExtension(path);
        if (fileExtension.compare("") == 0)
        {
            throw std::runtime_error("No file extension found in: " + path);
        }
        else if (fileExtension.compare(".obj"))
        {
            std::cout << "obj file" << std::endl;
            loadFromObj(path, verticesColor);
        }
        else
        {
            throw std::runtime_error("File extension " + fileExtension + " is not supported.");
        }

        createVertexBuffer(device);
        createIndexBuffer(device);
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
                Vertex vertex = {};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

                if (index.texcoord_index >= 0)
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip the vertical component (.obj files assume 0 means bottom of immage, we assume it means top)
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
        }
    }

    void createVertexBuffer(std::shared_ptr<core::Device> device)
    {
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(vertices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        vertexBuffer = core::Buffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        // TODO: Group buffer copies in a single cb
        device->commandpools.transfer.execute([&](vk::CommandBuffer cb) {
            stagingBuffer.copyTo(cb, vertexBuffer, bufferSize);
        });
    }

    void createIndexBuffer(std::shared_ptr<core::Device> device)
    {
        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(indices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        indexBuffer = core::Buffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        device->commandpools.transfer.execute([&](vk::CommandBuffer cb) {
            stagingBuffer.copyTo(cb, indexBuffer, bufferSize);
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