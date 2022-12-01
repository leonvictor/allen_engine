#pragma once

#include <string>

#include <reflection/reflection.hpp>

#include "../editor_graph_node.hpp"

namespace aln
{

/// @brief Output node, receives the final pose to use in the frame
class PoseEditorNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE();

  public:
    virtual void Serialize() override
    {
        // TODO
    }

    virtual void Initialize() override
    {
        m_name = "Output Pose";
        AddInputPin(PinValueType::Pose);
    }
};
} // namespace aln