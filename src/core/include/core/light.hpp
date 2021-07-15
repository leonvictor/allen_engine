#pragma once

#include <entities/spatial_component.hpp>
#include <vulkan/vulkan.hpp>

#include <glm/vec3.hpp>

namespace aln
{
struct LightUniform
{
    alignas(16) glm::vec4 position;  // position.w represents type of light
    alignas(16) glm::vec3 direction; //direction.w represents range
    alignas(16) glm::vec3 color;     // color.w represents intensity
    // TODO: Add inner and outer cutoff for spot lights
};

class Light : public entities::SpatialComponent
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

    LightUniform GetUniform();

    /// @brief Returns the vulkan bindings representing a light.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();

    std::string GetComponentTypeName() { return "Light"; }
};
} // namespace aln