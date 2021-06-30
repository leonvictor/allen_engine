#pragma once

#include "../spatial_component.hpp"
#include <vulkan/vulkan.hpp>

#include "../../graphics/device.hpp"
#include "../../graphics/resources/buffer.hpp"

struct LightUniform
{
    alignas(16) glm::vec4 position;  // position.w represents type of light
    alignas(16) glm::vec3 direction; //direction.w represents range
    alignas(16) glm::vec3 color;     // color.w represents intensity
    // TODO: Add inner and outer cutoff for spot lights
};

class Light : public SpatialComponent
{
  public:
    enum class Type
    {
        Directionnal,
        Spot,
        Point
    };

    Type type;

    glm::vec3 direction;
    glm::vec3 color;

    float intensity = 1.0f;
    float range = 1.0f;

    void Initialize() override {}
    void Shutdown() override {}
    bool Load() override { return true; } // Lights do not need to load resources
    void Unload() override {}

    LightUniform GetUniform()
    {
        Transform t = GetWorldTransform();

        LightUniform u;
        u.position = glm::vec4(t.position, (float) type);
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