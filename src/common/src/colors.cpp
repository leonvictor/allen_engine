#include "colors.hpp"

#include <maths/maths.hpp>

namespace aln
{
/// @todo
const RGBColor RGBColor::Red = RGBColor(178, 34, 34);
const RGBColor RGBColor::Pink = RGBColor(255, 0, 127);
const RGBColor RGBColor::Orange = RGBColor();
const RGBColor RGBColor::Blue = RGBColor(102, 195, 255);
const RGBColor RGBColor::Green = RGBColor(6, 214, 160);
const RGBColor RGBColor::Yellow = RGBColor(255, 191, 71);
const RGBColor RGBColor::Purple = RGBColor();
const RGBColor RGBColor::Black = RGBColor(0, 0, 0);
const RGBColor RGBColor::White = RGBColor(255, 255, 255);

HSVColor::operator RGBColor() const { return RGBColor(RGBUnitColor(*this)); }

HSVColor::HSVColor(const RGBUnitColor& rgbUnitColor)
{
    /// @note https://www.cs.rit.edu/~ncs/color/t_convert.html
    float max = Maths::Max(rgbUnitColor.m_red, Maths::Max(rgbUnitColor.m_green, rgbUnitColor.m_blue));
    float min = Maths::Min(rgbUnitColor.m_red, Maths::Min(rgbUnitColor.m_green, rgbUnitColor.m_blue));
    float delta = max - min;

    m_value = max;

    if (max != 0)
    {
        m_saturation = delta / max;
    }
    else
    {
        m_hue = -1.0f;
        m_saturation = 0.0f;
        return; // Undefined, achromatic grey
    }

    if (max == rgbUnitColor.m_red)
    {
        m_hue = (rgbUnitColor.m_green - rgbUnitColor.m_blue) / delta;
    }
    else if (max == rgbUnitColor.m_green)
    {
        m_hue = 2 + (rgbUnitColor.m_blue - rgbUnitColor.m_red) / delta;
    }
    else
    {
        m_hue = 4 + (rgbUnitColor.m_red - rgbUnitColor.m_green) / delta;
    }

    m_hue = m_hue * 60.0f;
    if (m_hue < 0.0f)
    {
        m_hue += 360.0f;
    }
}

HSVColor::HSVColor(const RGBColor& rgbColor) : HSVColor(static_cast<RGBUnitColor>(rgbColor)) {}

RGBUnitColor::RGBUnitColor(const HSVColor& hsvColor)
{
    /// @note https://www.cs.rit.edu/~ncs/color/t_convert.html
    if (hsvColor.m_saturation == 0.0f) // Achromatic grey
    {
        m_red = hsvColor.m_value;
        m_green = hsvColor.m_value;
        m_blue = hsvColor.m_value;
    }

    float h = hsvColor.m_hue / 60;
    uint8_t i = Maths::Floor(h);
    float f = h - i; // factorial part of h
    float p = hsvColor.m_value * (1 - hsvColor.m_saturation);
    float q = hsvColor.m_value * (1 - hsvColor.m_saturation * f);
    float t = hsvColor.m_value * (1 - hsvColor.m_saturation * (1 - f));

    switch (i)
    {
    case 0:
        m_red = hsvColor.m_value;
        m_green = t;
        m_blue = p;
    case 1:
        m_red = q;
        m_green = hsvColor.m_value;
        m_blue = p;
    case 2:
        m_red = p;
        m_green = hsvColor.m_value;
        m_blue = t;
    case 3:
        m_red = p;
        m_green = q;
        m_blue = hsvColor.m_value;
    case 4:
        m_red = t;
        m_green = p;
        m_blue = hsvColor.m_value;
    case 5:
        m_red = hsvColor.m_value;
        m_green = p;
        m_blue = q;
    }
    assert(false);
}

RGBColor::RGBColor(const HSVColor& hsvColor) : RGBColor(RGBUnitColor(hsvColor)) {}

RGBUnitColor::RGBUnitColor(const RGBColor& rgbColor) : m_red(rgbColor.m_red / 255.0f), m_green(rgbColor.m_green / 255.0f), m_blue(rgbColor.m_blue / 255.0f) {}
RGBUnitColor::operator RGBColor() const { return RGBColor(*this); }

RGBAUnitColor::RGBAUnitColor(const RGBAColor& rgbaColor) : m_red(rgbaColor.m_red / 255.0f), m_green(rgbaColor.m_green / 255.0f), m_blue(rgbaColor.m_blue / 255.0f), m_alpha(rgbaColor.m_alpha / 255.0f) {}
RGBAUnitColor::operator RGBAColor() const { return RGBAColor(*this); }

} // namespace aln