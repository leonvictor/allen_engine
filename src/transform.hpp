#pragma once

#include "components.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform : public Component
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat();
    glm::vec3 scale = glm::vec3(1.0f);
};