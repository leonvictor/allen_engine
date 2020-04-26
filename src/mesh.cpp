#include<vector>
#include<unordered_map>

#include "vertex.hpp"
#include "core/device.hpp"
#include "core/buffer.cpp"
#include "core/context.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    core::Buffer vertexBuffer;
    core::Buffer indexBuffer;

    glm::vec3 position; // TODO: This doesn't belong here
    glm::vec3 rotation; // Neither does this

    glm::mat4 getModelMatrix() {
        return glm::translate(modelMatrix, position);
    }

    std::shared_ptr<core::Context> context;
    std::shared_ptr<core::Device> device;

    Mesh() {}

    Mesh(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device) {
        this->context = context;
        this->device = device;
    }

    static Mesh fromObj(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path, glm::vec3 position = glm::vec3(0.0f)) {

            Mesh mesh(context, device);

            mesh.position = position; // Initially place the object at the center of the scene

            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex = {};
                    
                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };
                    
                    if (index.texcoord_index >= 0) {
                        vertex.texCoord = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip the vertical component (.obj files assume 0 means bottom of immage, we assume it means top)
                        };
                    }

                    vertex.color = {1.0f, 1.0f, 1.0f};
                    
                    if (index.normal_index >= 0) {
                        vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2],
                        };
                    }

                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
                        mesh.vertices.push_back(vertex);
                    }
                    
                    mesh.indices.push_back(uniqueVertices[vertex]);
                }
            }

            mesh.createVertexBuffer();
            mesh.createIndexBuffer();
            return mesh;
    }

    void createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(vertices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        // vertexBuffer = std::make_shared<core::Buffer>(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        vertexBuffer = core::Buffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        
        context->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createIndexBuffer() {
        vk::DeviceSize  bufferSize = sizeof(indices[0]) * indices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(indices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        indexBuffer = core::Buffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        context->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        stagingBuffer.destroy();
    }

    void destroy() {
        vertexBuffer.destroy();
        indexBuffer.destroy();
    }
    
private:
    glm::mat4 modelMatrix = glm::mat4(1.0f); // TODO
};