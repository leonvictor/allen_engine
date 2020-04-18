#pragma once

// TODO: Include vulkan directly...
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#include <array>

class Vertex {
    public: 
        alignas(16) glm::vec3 pos;
        alignas(16) glm::vec3 color;
        alignas(16) glm::vec2 texCoord;
        alignas(16) glm::vec3 normal;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescription() {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescription = {};
            attributeDescription[0].binding = 0;
            attributeDescription[0].location = 0;
            attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[0].offset = offsetof(Vertex, pos);
            attributeDescription[1].binding = 0;
            attributeDescription[1].location = 1;
            attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[1].offset = offsetof(Vertex, color);
            attributeDescription[2].binding = 0;
            attributeDescription[2].location = 2;
            attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescription[2].offset = offsetof(Vertex, texCoord);
            attributeDescription[3].binding = 0;
            attributeDescription[3].location = 3;
            attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[3].offset = offsetof(Vertex, normal);
            return attributeDescription;
        }

        bool operator==(const Vertex& other) const {
            return pos == other.pos && color == other.color && texCoord == other.texCoord;
        }

        bool operator!=(const Vertex& other) const {
            return !operator==(other);
        }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return (
                (hash<glm::vec3>()(vertex.pos) ^ 
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ 
                (hash<glm::vec2>()(vertex.texCoord) << 1);    
        }
    };
}