#pragma once

#include <glm/glm.hpp>
#include "../core/buffer.cpp"
#include <vulkan/vulkan.hpp>

namespace ecs::components {
    struct Transform {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
    };

    struct Renderable {
        bool display = true;
        core::Buffer uniform;
        vk::DescriptorSet descriptorSet;
    };
}