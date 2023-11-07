#include "components/light.hpp"

#include <common/transform.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, Light)
ALN_REFLECT_MEMBER(m_color)
ALN_REFLECT_MEMBER(m_intensity)
ALN_REGISTER_IMPL_END()

LightUniform Light::GetUniform()
{
    Transform t = GetWorldTransform();

    LightUniform u;
    u.position = Vec4(t.GetTranslation(), (float) m_type);
    u.direction = Vec4(m_direction, m_range);
    u.color = Vec4(static_cast<Vec3>(m_color), m_intensity);

    return u;
}

Vector<vk::DescriptorSetLayoutBinding> Light::GetDescriptorSetLayoutBindings()
{
    Vector<vk::DescriptorSetLayoutBinding> bindings{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        }};

    return bindings;
}

} // namespace aln