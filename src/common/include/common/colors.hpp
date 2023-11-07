#pragma once

#include <aln_common_export.h>

#include "maths/vec3.hpp"
#include "maths/vec4.hpp"

namespace aln
{

// fwd
class HSVColor;
class RGBColor;
class RGBAColor;
class RGBUnitColor;
class RGBAUnitColor;

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
    explicit HSVColor(const RGBUnitColor& unitRgbColor);
    explicit HSVColor(const RGBColor& rgbColor);

    explicit operator RGBUnitColor() const;
    explicit operator RGBColor() const;
};

class ALN_COMMON_EXPORT RGBUnitColor
{
    friend class RGBColor;
    friend class HSVColor;

  public:
    float m_red;
    float m_green;
    float m_blue;

  public:
    RGBUnitColor() = default;
    RGBUnitColor(float red, float green, float blue) : m_red(red), m_green(green), m_blue(blue)
    {
        assert(red >= 0.0f && red <= 1.0f);
        assert(green >= 0.0f && green <= 1.0f);
        assert(blue >= 0.0f && blue <= 1.0f);
    }
    explicit RGBUnitColor(const RGBColor& rgbColor);
    explicit RGBUnitColor(const HSVColor& hsvColor);
    explicit RGBUnitColor(const Vec3& vec) : RGBUnitColor(vec.x, vec.y, vec.z) {}

    explicit operator RGBColor() const;
    explicit operator HSVColor() const { return HSVColor(*this); }
    explicit operator Vec3() const { return {m_red, m_green, m_blue}; }
};

class ALN_COMMON_EXPORT RGBAUnitColor
{
    friend class RGBAColor;

  public:
    float m_red;
    float m_green;
    float m_blue;
    float m_alpha;

  public:
    RGBAUnitColor() = default;
    RGBAUnitColor(float red, float green, float blue, float alpha) : m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
    {
        assert(m_red >= 0.0f && m_red <= 1.0f);
        assert(m_green >= 0.0f && m_green <= 1.0f);
        assert(m_blue >= 0.0f && m_blue <= 1.0f);
        assert(m_alpha >= 0.0f && m_alpha <= 1.0f);
    }
    explicit RGBAUnitColor(const RGBAColor& rgbaColor);
    explicit RGBAUnitColor(const Vec4& vec) : RGBAUnitColor(vec.x, vec.y, vec.z, vec.w) {}

    explicit operator RGBAColor() const;
    explicit operator Vec4() const { return {m_red, m_green, m_blue, m_alpha}; }
};

class ALN_COMMON_EXPORT RGBColor
{
    friend class RGBUnitColor;

  public:
    uint8_t m_red;
    uint8_t m_green;
    uint8_t m_blue;

  public:
    RGBColor() = default;
    RGBColor(uint8_t red, uint8_t green, uint8_t blue) : m_red(red), m_green(green), m_blue(blue) {}
    explicit RGBColor(const RGBUnitColor& unitRGB) : m_red(unitRGB.m_red * 255), m_green(unitRGB.m_green * 255), m_blue(unitRGB.m_blue * 255) {}
    explicit RGBColor(const HSVColor& hsvColor);
    explicit RGBColor(const Vec3& vec) : RGBColor(static_cast<uint8_t>(vec.x), static_cast<uint8_t>(vec.y), static_cast<uint8_t>(vec.z))
    {
        assert(vec.x >= 0.0f && vec.x <= 255.0f);
        assert(vec.y >= 0.0f && vec.y <= 255.0f);
        assert(vec.z >= 0.0f && vec.z <= 255.0f);
    }

    explicit operator RGBUnitColor() const { return RGBUnitColor(*this); }
    explicit operator HSVColor() const { return HSVColor(*this); };
    explicit operator Vec3() const { return {(float) m_red, (float) m_green, (float) m_blue}; }
    explicit operator uint32_t() const { return m_red | m_green << 8 | m_blue << 16 | 255 << 24; }

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
    friend class RGBAUnitColor;

  public:
    uint8_t m_red;
    uint8_t m_green;
    uint8_t m_blue;
    uint8_t m_alpha;

  public:
    RGBAColor() = default;
    RGBAColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : m_red(r), m_green(g), m_blue(b), m_alpha(a) {}
    explicit RGBAColor(const RGBAUnitColor& unitRGBA) : m_red(unitRGBA.m_red * 255), m_green(unitRGBA.m_green * 255), m_blue(unitRGBA.m_blue * 255), m_alpha(unitRGBA.m_alpha * 255) {}

    explicit operator RGBAUnitColor() const { return RGBAUnitColor(*this); }
    explicit operator uint32_t() const { return m_red | m_green << 8 | m_blue << 16 | m_alpha << 24; }
};

static_assert(std::is_trivial_v<RGBAColor>);
static_assert(std::is_trivial_v<RGBColor>);

} // namespace aln