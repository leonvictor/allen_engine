#include "transform.hpp"

namespace aln
{
Transform& Transform::operator=(const Transform& other)
{
    position = other.position;
    rotation = other.rotation;
    scale = other.scale;
    return *this;
}

bool operator==(const Transform& a, const Transform& b)
{
    return a.position == b.position && a.rotation == b.rotation && a.scale == b.scale;
}

bool operator!=(const Transform& lhs, const Transform& rhs)
{
    return !(lhs == rhs);
}

glm::mat4 Transform::GetModelMatrix() const
{
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    return model;
}
} // namespace aln