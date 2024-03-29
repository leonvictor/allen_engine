#pragma once

#include <common/maths/vec2.hpp>
#include <common/maths/vec3.hpp>
#include <common/maths/vec4.hpp>

#include <common/containers/array.hpp>

namespace aln
{

#pragma warning(disable : 4324)

struct Vertex
{
  public:
    alignas(16) Vec3 pos;
    alignas(16) Vec3 color;
    alignas(16) Vec2 texCoord;
    alignas(16) Vec3 normal;

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
    alignas(16) Vec3 pos;
    alignas(16) Vec3 color;
    alignas(16) Vec2 texCoord;
    alignas(16) Vec3 normal;
    alignas(16) Vec4 weights;
    alignas(16) Array<uint32_t, 4> boneIndices;
};

struct DebugVertex
{
    alignas(16) Vec3 pos;
    alignas(16) Vec3 color;
};

#pragma warning(default : 4324)

} // namespace aln
