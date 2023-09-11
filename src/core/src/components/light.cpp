#include "components/light.hpp"

#include <common/transform.hpp>
#include <glm/vec4.hpp>

namespace aln
{

LightUniform Light::GetUniform()
{
    Transform t = GetWorldTransform();

    LightUniform u;
    u.position = glm::vec4(t.GetTranslation(), (float) type);
    u.direction = glm::vec4(direction, range);
    u.color = glm::vec4(m_color.Vec3(), intensity);

    return u;
}

std::vector<vk::DescriptorSetLayoutBinding> Light::GetDescriptorSetLayoutBindings()
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
} // namespace aln

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::Light)
ALN_REFLECT_MEMBER(m_color, Color)
ALN_REFLECT_MEMBER(intensity, Intensity)
ALN_REGISTER_IMPL_END()