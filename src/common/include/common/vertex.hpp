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

struct SkinnedVertex : public Vertex
{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(16) glm::vec2 texCoord;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec4 weights;
    alignas(16) glm::vec4 boneIndices;
};

struct DebugVertex
{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
};

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