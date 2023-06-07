#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <aln_common_export.h>

// TODO: Move to core
namespace aln
{

/// @brief Represents an RGB Color
/// @todo vec3 is overkill, colors can be represented on 3*8bits
class ALN_COMMON_EXPORT RGBColor : public glm::vec3
{
    static_assert(std::is_trivial_v<glm::vec3>);

  public:
    RGBColor() = default;
    RGBColor(float r, float g, float b) : glm::vec3(r, g, b) {}

    /// @brief Convert to a 32-bit integer
    uint32_t U32() const {
        uint32_t out;
        out = ((uint32_t) (this->x * 255.0f + 0.5f));
        out |= ((uint32_t) (this->y * 255.0f + 0.5f)) << 8;
        out |= ((uint32_t) (this->z * 255.0f + 0.5f)) << 16;
        out |= (uint32_t) 255 << 24;
        return out;
    }

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
    RGBAColor() = default;
    RGBAColor(float r, float g, float b, float a) : glm::vec4(r, g, b, a) {}
};

static_assert(std::is_trivial_v<RGBAColor>);
static_assert(std::is_trivial_v<RGBColor>);

} // namespace aln