#pragma once

#include <common/maths/rectangle.hpp>

namespace aln
{
class Viewport
{
  public:
    Rectangle m_size;    

  public:
    float GetAspectRatio() const { return (float) m_size.width / (float) m_size.height; }
};
} // namespace aln