#pragma once

#include "graphics/device.hpp"
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

enum LightType
{
    Directionnal,
    Spot,
    Point
};

struct LightUniform
{
    alignas(16) glm::vec4 position;  // position.w represents type of light
    alignas(16) glm::vec3 direction; //direction.w represents range
    alignas(16) glm::vec3 color;     // color.w represents intensity
    // TODO: Add inner and outer cutoff for spot lights
};

class Light
{
  public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;

    float intensity = 1.0f;
    float range = 1.0f;

    LightType type;

    LightUniform getUniform()
    {
        LightUniform u;
        u.position = glm::vec4(position, (float) type);
        u.direction = glm::vec4(direction, range);
        u.color = glm::vec4(color, intensity);

        return u;
    }

    /// @brief Returns the vulkan bindings representing a light.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            vk::DescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            }};

        return bindings;
    }
};