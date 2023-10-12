#include "transform.hpp"

#include "maths/matrix4x4.hpp"

namespace aln
{

Matrix4x4 Transform::ToMatrix() const
{
    return m_translation.ToTranslationMatrix() * m_rotation.ToMatrix() * m_scale.ToScalingMatrix();
}

Transform Transform::GetInverse() const
{
    Transform inverse;
    inverse.m_scale.x = Maths::SafeDivide(1.0f, m_scale.x);
    inverse.m_scale.y = Maths::SafeDivide(1.0f, m_scale.y);
    inverse.m_scale.z = Maths::SafeDivide(1.0f, m_scale.z);
    inverse.m_rotation = m_rotation.Inversed();
    inverse.m_translation = inverse.m_rotation.RotateVector(inverse.m_scale.Scale(m_translation) * - 1.0f);

    return inverse;
}

bool Transform::operator==(const Transform& b) const
{
    return m_translation.IsNearEqual(b.m_translation) &&
           m_rotation.IsNearEqual( b.m_rotation) &&
           m_scale.IsNearEqual(b.m_scale);
}

bool Transform::operator!=(const Transform& rhs) const
{
    return !(*this == rhs);
}

Transform& Transform::operator*=(const Transform& b)
{
    m_translation = m_translation + m_rotation.RotateVector(m_scale.Scale(b.m_translation));
    m_rotation = (m_rotation * b.m_rotation).Normalized(); // glm quaternion multiplication is the same as matrices
    m_scale = m_scale.Scale(b.m_scale);
    return *this;
}

Transform Transform::operator*(const Transform& b) const
{
    Transform res = *this;
    res *= b;
    return res;
}

const Transform Transform::Identity = Transform();

} // namespace aln