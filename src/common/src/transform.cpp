#include "transform.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace aln
{
Transform::Transform(const glm::mat4x4& matrix)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, m_scale, m_rotation, m_translation, skew, perspective);
}

glm::mat4x4 Transform::ToMatrix() const
{
    return glm::translate(m_translation) * glm::toMat4(m_rotation) * glm::scale(m_scale);
}

Transform Transform::GetInverse() const
{
    Transform inverse;

    inverse.m_scale.x = std::abs(m_scale.x) < Epsilon ? 0.0f : 1.0f / m_scale.x;
    inverse.m_scale.y = std::abs(m_scale.y) < Epsilon ? 0.0f : 1.0f / m_scale.y;
    inverse.m_scale.z = std::abs(m_scale.z) < Epsilon ? 0.0f : 1.0f / m_scale.z;

    inverse.m_rotation = glm::inverse(m_rotation);

    inverse.m_translation = glm::rotate(inverse.m_rotation, inverse.m_scale * m_translation * -1.0f);

    return inverse;
}

Transform& Transform::operator=(const Transform& other)
{
    m_translation = other.m_translation;
    m_rotation = other.m_rotation;
    m_scale = other.m_scale;
    return *this;
}

bool Transform::operator==(const Transform& b) const
{
    return glm::all(glm::epsilonEqual(m_translation, b.m_translation, Epsilon)) &&
           glm::all(glm::epsilonEqual(m_rotation, b.m_rotation, Epsilon)) &&
           glm::all(glm::epsilonEqual(m_scale, b.m_scale, Epsilon));
}

bool Transform::operator!=(const Transform& rhs) const
{
    return !(*this == rhs);
}

void Transform::operator*=(const Transform& b)
{
    m_rotation = m_rotation * b.m_rotation;
    m_rotation = glm::normalize(m_rotation);
    m_translation = glm::rotate(b.m_rotation, m_translation * b.m_scale) + b.m_translation;
    m_scale = m_scale * b.m_scale;
}

Transform Transform::operator*(const Transform& b) const
{
    Transform res = *this;
    res *= b;
    return res;
}

const Transform Transform::Identity = Transform();

} // namespace aln