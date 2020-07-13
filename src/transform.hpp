#pragma once

#include "components.hpp"
#include <glm/glm.hpp>

struct Transform : public Component
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};