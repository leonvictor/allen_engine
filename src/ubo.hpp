#pragma once

#include <glm/glm.hpp>

namespace vkg
{
struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 cameraPos;
};
} // namespace vkg