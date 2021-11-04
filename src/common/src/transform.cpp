#include "transform.hpp"

namespace aln
{
Transform& Transform::operator=(const Transform& other)
{
    m_position = other.m_position;
    m_rotation = other.m_rotation;
    m_scale = other.m_scale;
    m_rotationEuler = other.m_rotationEuler;
    return *this;
}

bool Transform::operator==(const Transform& b)
{
    return m_position == b.m_position && m_rotation == b.m_rotation && m_scale == b.m_scale;
}

bool Transform::operator!=(const Transform& rhs)
{
    return !(*this == rhs);
}
}