#pragma once

#include <common/colors.hpp>
#include <common/maths/vec3.hpp>
#include <common/maths/vec4.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/type_info.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{
struct LightUniform
{
    alignas(16) Vec4 position;  // position.w represents type of light
    alignas(16) Vec4 direction; // direction.w represents range
    alignas(16) Vec4 color;     // color.w represents intensity
    // TODO: Add inner and outer cutoff for spot lights
};

class LightComponent : public SpatialComponent
{
    ALN_REGISTER_TYPE();

  public:
    enum class Type
    {
        Directional,
        Spot,
        Point
    };

    Type m_type;

    Vec3 m_direction;
    RGBColor m_color = RGBColor::White;

    float m_intensity = 1.0f;
    float m_range = 1.0f;

    void Initialize() override {}
    void Shutdown() override {}
    void Load(const LoadingContext& loadingContext) override {} // Lights do not need to load resources
    void Unload(const LoadingContext& loadingContext) override {}

    LightUniform GetUniform();

    /// @brief Returns the vulkan bindings representing a light.
    static Vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();
};
} // namespace aln