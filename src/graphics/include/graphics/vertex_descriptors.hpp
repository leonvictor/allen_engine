#pragma once

#include "vulkan/vulkan.hpp"
#include <common/vertex.hpp>

#include <vector>

namespace aln::vkg
{

template <typename T>
struct VertexDescriptor
{
    static vk::VertexInputBindingDescription GetBindingDescription() {}
    static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescription() {}
};

// TODO: Maybe specialize the template on Mesh type rather than Vertex type
template <>
struct VertexDescriptor<aln::Vertex>
{
    // TODO: Find a good way to get the correct binding descriptions in derived Vertex classes (eg SkinnedVertex)
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex); // We would only need to modify this if the method wasn't static
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        std::vector<vk::VertexInputAttributeDescription> attributeDescription(4);

        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescription[0].offset = offsetof(Vertex, pos);

        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescription[1].offset = offsetof(Vertex, color);

        attributeDescription[2].binding = 0;
        attributeDescription[2].location = 2;
        attributeDescription[2].format = vk::Format::eR32G32Sfloat;
        attributeDescription[2].offset = offsetof(Vertex, texCoord);

        attributeDescription[3].binding = 0;
        attributeDescription[3].location = 3;
        attributeDescription[3].format = vk::Format::eR32G32B32Sfloat;
        attributeDescription[3].offset = offsetof(Vertex, normal);

        return attributeDescription;
    }
};
} // namespace aln::vkg