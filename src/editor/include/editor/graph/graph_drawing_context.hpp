#pragma once

#include "../reflected_types/reflected_type_editor.hpp"

#include <anim/graph/value_types.hpp>
#include <common/colors.hpp>

namespace aln
{
struct GraphDrawingContext
{
    struct ColorScheme
    {
        RGBColor m_defaultColor;
        RGBColor m_hoveredColor;
        RGBColor m_selectedColor;
    };

    float m_currentNodeWidth = 0.0f;
    ReflectedTypeEditor m_nodeInspector;

    static RGBColor GetTypeColor(NodeValueType valueType)
    {
        // TODO: Handle all possible types
        // TODO: Decide on a cool color palette
        switch (valueType)
        {
        case NodeValueType::Pose:
            return RGBColor::Pink;
        case NodeValueType::Float:
            return RGBColor::Yellow;
        case NodeValueType::Bool:
            return RGBColor::Green;
        case NodeValueType::ID:
            return RGBColor::Blue;
        case NodeValueType::Unknown:
        default:
            assert(false); // Is the value type handled ?
            return RGBColor::Black;
        }
    }

    static ColorScheme GetTypeColorScheme(const NodeValueType& valueType) {
        auto defaultColor = GetTypeColor(valueType);

        auto hsvColor = defaultColor.ToHSV();
        hsvColor.m_saturation -= 0.2;
        auto alteredColor = hsvColor.ToRGB();
        
        ColorScheme scheme;
        scheme.m_defaultColor = defaultColor;
        scheme.m_selectedColor = alteredColor;
        scheme.m_hoveredColor = alteredColor;

        return scheme;
    }
};
} // namespace aln