#pragma once

#include <glm/gtx/hash.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace aln
{
struct Vertex
{
  public:
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec2 texCoord;
    alignas(16) glm::vec3 normal;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }

    bool operator!=(const Vertex& other) const
    {
        return !operator==(other);
    }
};

// class SkinnedVertex : public Vertex
// {
//   public:
//     alignas(16) glm::vec4 jointIndices;
//     alignas(16) glm::vec4 jointWeights;

//     // TODO: Find a good way to get the correct binding descriptions in derived Vertex classes (eg SkinnedVertex)
//     static vk::VertexInputBindingDescription getBindingDescription()
//     {
//         vk::VertexInputBindingDescription bindingDescription = {};
//         bindingDescription.binding = 0;
//         bindingDescription.stride = sizeof(SkinnedVertex); // We would only need to modify this if the method wasn't static
//         bindingDescription.inputRate = vk::VertexInputRate::eVertex;
//         return bindingDescription;
//     }

//     static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescription()
//     {
//         std::array<vk::VertexInputAttributeDescription, 4> attributeDescription = {};
//         attributeDescription[0].binding = 0;
//         attributeDescription[0].location = 0;
//         attributeDescription[0].format = vk::Format::eR32G32B32Sfloat;
//         attributeDescription[0].offset = offsetof(SkinnedVertex, pos);
//         attributeDescription[1].binding = 0;
//         attributeDescription[1].location = 1;
//         attributeDescription[1].format = vk::Format::eR32G32B32Sfloat;
//         attributeDescription[1].offset = offsetof(SkinnedVertex, color);
//         attributeDescription[2].binding = 0;
//         attributeDescription[2].location = 2;
//         attributeDescription[2].format = vk::Format::eR32G32Sfloat;
//         attributeDescription[2].offset = offsetof(SkinnedVertex, texCoord);
//         attributeDescription[3].binding = 0;
//         attributeDescription[3].location = 3;
//         attributeDescription[3].format = vk::Format::eR32G32B32Sfloat;
//         attributeDescription[3].offset = offsetof(SkinnedVertex, normal);
//         attributeDescription[4].location = 4;
//         attributeDescription[4].format = vk::Format::eR32G32B32Sfloat;
//         attributeDescription[4].offset = offsetof(SkinnedVertex, jointIndices);
//         attributeDescription[5].location = 5;
//         attributeDescription[5].format = vk::Format::eR32G32B32Sfloat;
//         attributeDescription[5].offset = offsetof(SkinnedVertex, jointWeights);
//         return attributeDescription;
//     }
// };
} // namespace aln

namespace std
{
template <>
struct hash<aln::Vertex>
{
    size_t operator()(aln::Vertex const& vertex) const
    {
        return (
                   (hash<glm::vec3>()(vertex.pos) ^
                       (hash<glm::vec3>()(vertex.color) << 1)) >>
                   1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1) ^
               (hash<glm::vec3>()(vertex.normal));
    }
};
} // namespace std