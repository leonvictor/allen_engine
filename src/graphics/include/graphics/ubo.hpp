#pragma once

#include <common/maths/matrix4x4.hpp>
#include <common/maths/vec3.hpp>

namespace aln
{
struct UniformBufferObject
{
    alignas(16) Matrix4x4 model;
    alignas(16) Matrix4x4 view;
    alignas(16) Matrix4x4 projection;
    alignas(16) Vec3 cameraPos;
};
} // namespace vkg