#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <aln_common_export.h>

namespace aln
{

/// @brief Represents an RGB Color
/// @todo vec3 is overkill, colors can be represented on 3*8bits
class ALN_COMMON_EXPORT RGBColor : public glm::vec3
{
  public:
    RGBColor() : glm::vec3(0.0f, 0.0f, 0.0f) {}
    RGBColor(float r, float g, float b) : glm::vec3(r, g, b) {}

    static const RGBColor Red;
    static const RGBColor Pink;
    static const RGBColor Orange;
    static const RGBColor Blue;
    static const RGBColor Green;
    static const RGBColor Yellow;
    static const RGBColor Purple;
    static const RGBColor Black;
    static const RGBColor White;
};

class RGBAColor : public glm::vec4
{
  public:
    RGBAColor() : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) {}
    RGBAColor(float r, float g, float b, float a) : glm::vec4(r, g, b, a) {}
};
} // namespace aln