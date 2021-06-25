#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// TODO: rotation should probably be in quaternions or matrix format
// but we need to access it as euler angles. How do we handle this ?
// Simple getter/setter is not possible as we need to access values directly in imgui
struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat();
    glm::vec3 scale = glm::vec3(1.0f);
};