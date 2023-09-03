#include "colors.hpp"

#include <cmath>

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

RGBColor HSVColor::ToRGB() const
{
    /// @note https://www.cs.rit.edu/~ncs/color/t_convert.html
    if (m_saturation == 0) // Achromatic grey
    {
        return RGBColor(m_value, m_value, m_value);
    }

    float h = m_hue / 60;
    uint8_t i = std::floor(h);
    float f = h - i; // factorial part of h
    float p = m_value * (1 - m_saturation);
    float q = m_value * (1 - m_saturation * f);
    float t = m_value * (1 - m_saturation * (1 - f));

    switch (i)
    {
    case 0:
        return RGBColor(m_value, t, p);
    case 1:
        return RGBColor(q, m_value, p);
    case 2:
        return RGBColor(p, m_value, t);
    case 3:
        return RGBColor(p, q, m_value);
    case 4:
        return RGBColor(t, p, m_value);
    case 5:
        return RGBColor(m_value, p, q);
    }
}

HSVColor RGBColor::ToHSV() const
{
    /// @note https://www.cs.rit.edu/~ncs/color/t_convert.html
    float hue, saturation, value;

    float redf = m_red / 255.0f;
    float greenf = m_green / 255.0f;
    float bluef = m_blue / 255.0f;

    float max = std::max(redf, std::max(greenf, bluef));
    float min = std::min(redf, std::min(greenf, bluef));
    float delta = max - min;

    value = max;

    if (max != 0)
    {
        saturation = delta / max;
    }
    else
    {
        return HSVColor(-1.0f, 0, value); // Undefined, achromatic grey
    }

    if (max == redf)
    {
        hue = (greenf - bluef) / delta;
    }
    else if (max == greenf)
    {
        hue = 2 + (bluef - redf) / delta;
    }
    else
    {
        hue = 4 + (redf - greenf) / delta;
    }

    hue = hue * 60;
    if (hue < 0)
    {
        hue += 360;
    }

    return HSVColor(hue, saturation, value);
}
} // namespace aln