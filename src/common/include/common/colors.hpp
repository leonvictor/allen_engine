#pragma once

#include <aln_common_export.h>

#include "maths/vec3.hpp"
#include "maths/vec4.hpp"

namespace aln
{

// fwd
class RGBColor;

class ALN_COMMON_EXPORT HSVColor
{
  public:
    float m_hue;        // [0, 360]
    float m_saturation; // [0, 1]
    float m_value;      // [0, 1]

  public:
    HSVColor() = default;
    HSVColor(float hue, float saturation, float value) : m_hue(hue), m_saturation(saturation), m_value(value)
    {
        assert(m_hue >= 0.0f && m_hue <= 360.0f);
        assert(m_saturation >= 0.0f && m_saturation <= 1.0f);
        assert(m_value >= 0.0f && m_value <= 1.0f);
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

    Vec3 ToUnitRGB() const { return Vec3(m_red / 255.0f, m_green / 255.0f, m_blue / 255.0f); }

    static RGBColor FromUnitRGB(const Vec3& unitRGB)
    {
        assert(unitRGB.x >= 0.0f && unitRGB.x <= 1.0f);
        assert(unitRGB.y >= 0.0f && unitRGB.y <= 1.0f);
        assert(unitRGB.z >= 0.0f && unitRGB.z <= 1.0f);
        return RGBColor(unitRGB.x * 255, unitRGB.y * 255, unitRGB.z * 255);
    }

    HSVColor ToHSV() const;

    /// @brief Convert to a 32-bit integer
    uint32_t ToU32() const { return m_red | m_green << 8 | m_blue << 16 | 255 << 24; }

    Vec3 ToVec3() const { return {(float) m_red, (float) m_green, (float) m_blue}; }

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

class RGBAColor
{
  public:
    uint8_t m_red, m_green, m_blue, m_alpha;

    RGBAColor() = default;
    RGBAColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : m_red(r), m_green(g), m_blue(b), m_alpha(a) {}

    Vec4 ToVec4() const { return {(float) m_red, (float) m_green, (float) m_blue, (float) m_alpha}; }

    Vec4 ToUnitRGBA() const { return Vec4(m_red / 255.0f, m_green / 255.0f, m_blue / 255.0f, m_alpha / 255.0f); }

    static RGBAColor FromUnitRGBA(const Vec4& unitRGBA )
    {
        assert(unitRGBA.x >= 0.0f && unitRGBA.x <= 1.0f);
        assert(unitRGBA.y >= 0.0f && unitRGBA.y <= 1.0f);
        assert(unitRGBA.z >= 0.0f && unitRGBA.z <= 1.0f);
        assert(unitRGBA.w >= 0.0f && unitRGBA.w <= 1.0f);
        return RGBAColor(unitRGBA.x * 255, unitRGBA.y * 255, unitRGBA.z * 255, unitRGBA.w * 255);
    }
};

static_assert(std::is_trivial_v<RGBAColor>);
static_assert(std::is_trivial_v<RGBColor>);

} // namespace aln