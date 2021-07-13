#include "transform.hpp"

namespace aln::entities
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
}