#pragma once

#include <common/containers/vector.hpp>
#include <common/vertex.hpp>

#include "vulkan/vulkan.hpp"

namespace aln
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
        vk::VertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex,
        };

        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescription = {
            {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos)},
            {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
            {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)},
            {.location = 3, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal)},
        };

        return attributeDescription;
    }
}; // namespace aln

template <>
struct VertexDescriptor<SkinnedVertex>
{
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(SkinnedVertex),
            .inputRate = vk::VertexInputRate::eVertex,
        };

        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescription = {
            {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(SkinnedVertex, pos)},
            {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(SkinnedVertex, color)},
            {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(SkinnedVertex, texCoord)},
            {.location = 3, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(SkinnedVertex, normal)},
            {.location = 4, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(SkinnedVertex, weights)},
            {.location = 5, .binding = 0, .format = vk::Format::eR32G32B32A32Uint, .offset = offsetof(SkinnedVertex, boneIndices)},
        };

        return attributeDescription;
    }
};

template <>
struct VertexDescriptor<DebugVertex>
{
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(DebugVertex),
            .inputRate = vk::VertexInputRate::eVertex,
        };

        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> GetAttributeDescription()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescription = {
            {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(DebugVertex, pos)},
            {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(DebugVertex, color)},
        };

        return attributeDescription;
    }
};
} // namespace aln