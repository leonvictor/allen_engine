#include<vector>
#include<unordered_map>

#include "vertex.hpp"
#include "core/device.hpp"
#include "core/buffer.cpp"
#include "core/context.hpp"
#include "core/texture.cpp"
#include "material.cpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    core::Buffer vertexBuffer;
    core::Buffer indexBuffer;
    core::Buffer uniformBuffer;

    core::Texture texture;
    Material material;

    vk::DescriptorSet descriptorSet;

    // TODO: This deserve to be in a Transform class
    // Think about the design
    glm::vec3 position; // TODO: This doesn't belong here
    glm::vec3 rotation; // Neither does this
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 getModelMatrix() {
        return glm::translate(glm::scale(modelMatrix, scale), position);
    }

    std::shared_ptr<core::Context> context;
    std::shared_ptr<core::Device> device;
    
    Mesh() {}

    Mesh(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device) {
        this->context = context;
        this->device = device;
    }

    static Mesh fromObj(std::shared_ptr<core::Context> context, std::shared_ptr<core::Device> device, std::string path,
                glm::vec3 position = glm::vec3(0.0f), glm::vec3 color = {1.0f, 1.0f, 1.0f},
                MaterialBufferObject material = MaterialBufferObject(),
                std::string texturePath = "") {

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

                    vertex.color = color;
                    
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
            mesh.createUniformBuffer();
            
            mesh.addMaterial(material);
            
            if (!texturePath.empty()) {
                mesh.texture = core::Texture(context, device, texturePath);
            }
            return mesh;
    }

    void addMaterial(MaterialBufferObject newMaterial) {
        material = Material(device, newMaterial);
    }

    void createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        
        core::Buffer stagingBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        stagingBuffer.map(0, bufferSize);
        stagingBuffer.copy(vertices.data(), (size_t) bufferSize);
        stagingBuffer.unmap();

        vertexBuffer = core::Buffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);   
        
        context->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        stagingBuffer.destroy();
    }

    void createDescriptorSet(vk::DescriptorPool& descriptorPool, vk::DescriptorSetLayout& descriptorSetLayout) {
        // TODO: Make sure setLayout is already initialized
        vk::DescriptorSetAllocateInfo allocInfo{ descriptorPool, 1, &descriptorSetLayout };
        descriptorSet = device->logicalDevice.allocateDescriptorSets(allocInfo)[0];

        std::array<vk::WriteDescriptorSet, 3> writeDescriptors = {};

        writeDescriptors[0].dstSet = descriptorSet;
        writeDescriptors[0].dstBinding = 0; // Binding index 
        writeDescriptors[0].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[0].descriptorCount = 1;
        auto uniformDescriptor = uniformBuffer.getDescriptor();
        writeDescriptors[0].pBufferInfo = &uniformDescriptor;

        writeDescriptors[1].dstSet = descriptorSet;
        writeDescriptors[1].dstBinding = 1; // Binding index 
        writeDescriptors[1].dstArrayElement = 0; // Descriptors can be arrays: first index that we want to update
        writeDescriptors[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescriptors[1].descriptorCount = 1;
        auto textureDescriptor = texture.getDescriptor();
        writeDescriptors[1].pImageInfo = &textureDescriptor;

        // TODO: Materials presumably don't change so they don't need a binding
        writeDescriptors[2].dstSet = descriptorSet;
        writeDescriptors[2].dstBinding = 2;
        writeDescriptors[2].dstArrayElement = 0;
        writeDescriptors[2].descriptorType = vk::DescriptorType::eUniformBuffer;
        writeDescriptors[2].descriptorCount = 1;
        auto materialDescriptor = material.getBufferDescriptor();
        writeDescriptors[2].pBufferInfo = &materialDescriptor; // TODO: Replace w/ push constants ?
            
        device->logicalDevice.updateDescriptorSets(static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
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

    void createUniformBuffer() {
        uniformBuffer = core::Buffer(device, sizeof(core::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }

    void updateUniformBuffers(core::UniformBufferObject ubo) { 
        uniformBuffer.map(0, sizeof(ubo));
        uniformBuffer.copy(&ubo, sizeof(ubo));
        uniformBuffer.unmap();
    }

    void destroy() {
        vertexBuffer.destroy();
        indexBuffer.destroy();
        uniformBuffer.destroy();
        material.destroy();
        texture.destroy();
    }
    
    void revertNormals() {
        for (Vertex v : vertices) {
            v.normal = - v.normal;
        }
    }
private:
    glm::mat4 modelMatrix = glm::mat4(1.0f); // TODO
};