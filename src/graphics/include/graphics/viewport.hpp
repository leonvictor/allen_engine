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
    float GetWidth() const { return m_size.width; }
    float GetHeight() const { return m_size.height; }
    const Rectangle& GetSize() const { return m_size; }
};
} // namespace aln