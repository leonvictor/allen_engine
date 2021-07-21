#include "light.hpp"

#include <entities/transform.hpp>
#include <glm/vec4.hpp>

namespace aln
{
using namespace entities;

LightUniform Light::GetUniform()
{
    Transform t = GetWorldTransform();

    LightUniform u;
    u.position = glm::vec4(t.position, (float) type);
    u.direction = glm::vec4(direction, range);
    u.color = glm::vec4(color, intensity);

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

ALN_REGISTER_IMPL_BEGIN(aln::Light)
ALN_REFLECT_MEMBER(color)
ALN_REFLECT_MEMBER(intensity)
ALN_REGISTER_IMPL_END()