#pragma once

#include "../reflected_types/reflected_type_editor.hpp"

#include <anim/graph/value_types.hpp>
#include <common/colors.hpp>

namespace aln
{
struct GraphDrawingContext
{
    float m_currentNodeWidth = 0.0f;
    ReflectedTypeEditor m_nodeInspector;

    static RGBColor GetTypeColor(NodeValueType valueType)
    {
        // TODO: Handle all possible types
        // TODO: Handle hue variation on selected / hovered
        // TODO: Decide on a cool color palette
        switch (valueType)
        {
        case NodeValueType::Pose:
            return RGBColor::Pink;
        case NodeValueType::Float:
            return RGBColor::Yellow;
        case NodeValueType::Bool:
            return RGBColor::Orange;
        case NodeValueType::ID:
            return RGBColor::Blue;
        case NodeValueType::Unknown:
        default:
            assert(false); // Is the value type handled ?
            return RGBColor::Black;
        }
    }
};
} // namespace aln