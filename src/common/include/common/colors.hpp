#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <aln_common_export.h>

namespace aln
{

// fwd
class RGBColor;

class ALN_COMMON_EXPORT HSVColor
{
  private:
    float m_hue;        // [0, 1]
    float m_saturation; // [0, 1]
    float m_value;      // [0, 360]

  public:
    HSVColor() = default;
    HSVColor(float hue, float saturation, float value) : m_hue(hue), m_saturation(saturation), m_value(value)
    {
        assert(m_hue >= 0.0f && m_hue <= 1.0f);
        assert(m_saturation >= 0.0f && m_saturation <= 1.0f);
        assert(m_value >= 0.0f && m_value <= 360.0f);
    }

    RGBColor ToRGB() const;
};

class ALN_COMMON_EXPORT RGBColor
{
  private:
    uint8_t m_red;
    uint8_t m_green;
    uint8_t m_blue;

  public:
    RGBColor() = default;
    RGBColor(uint8_t red, uint8_t green, uint8_t blue) : m_red(red), m_green(green), m_blue(blue) {}

    HSVColor ToHSV() const;

    /// @brief Convert to a 32-bit integer
    uint32_t U32() const { return m_red | m_green << 8 | m_blue << 16 | 255 << 24; }

    glm::vec3 Vec3() const { return {m_red, m_green, m_blue}; }

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