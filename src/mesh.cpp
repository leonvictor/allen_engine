#include<vector>
#include<unordered_map>

#include "vertex.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 position; // TODO: This doesn't belong here
    glm::vec3 rotation; // Neither does this

    glm::mat4 getModelMatrix() {
        return glm::translate(modelMatrix, position);
    }

    static Mesh fromObj(std::string path, glm::vec3 position = glm::vec3(0.0f)) {

            Mesh mesh = {};
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
            return mesh;
    }

    // void createVertexBuffer() {
    //     VkDeviceSize  bufferSize = sizeof(vertices[0]) * vertices.size();
        
    //     VkBuffer stagingBuffer;
    //     VkDeviceMemory stagingBufferMemory;

    //     createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    //     void* data;
    //     vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    //     memcpy(data, model.vertices.data(), (size_t) bufferSize);
    //     vkUnmapMemory(device, stagingBufferMemory);

    //     createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        
    //     copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    //     vkDestroyBuffer(device, stagingBuffer, nullptr);
    //     vkFreeMemory(device, stagingBufferMemory, nullptr);
    // }
    
private:
    glm::mat4 modelMatrix = glm::mat4(1.0f); // TODO
};