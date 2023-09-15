#pragma once

#include "vulkan/vulkan.hpp"
#include <common/vertex.hpp>

#include <common/containers/vector.hpp>

namespace aln::vkg
{

template <typename T>
struct VertexDescriptor
{
    static vk::VertexInputBindingDescription GetBindingDescription() {}
    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription() {}
};

// TODO: Maybe specialize the template on Mesh type rather than Vertex type
template <>
struct VertexDescriptor<aln::Vertex>
{
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex); // We would only need to modify this if the method wasn't static
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescription(4);

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

template <>
struct VertexDescriptor<SkinnedVertex>
{
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(SkinnedVertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescription =
            {
                {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkinnedVertex, pos)},
                {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkinnedVertex, color)},
                {2, 0, vk::Format::eR32G32Sfloat, offsetof(SkinnedVertex, texCoord)},
                {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(SkinnedVertex, normal)},
                {4, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(SkinnedVertex, weights)},
                {5, 0, vk::Format::eR32G32B32A32Uint, offsetof(SkinnedVertex, boneIndices)},
            };

        return attributeDescription;
    }
};

template <>
struct VertexDescriptor<DebugVertex>
{
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(DebugVertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescription =
            {
                {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(DebugVertex, pos)},
                {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(DebugVertex, color)},
            };

        return attributeDescription;
    }
};
} // namespace aln::vkg