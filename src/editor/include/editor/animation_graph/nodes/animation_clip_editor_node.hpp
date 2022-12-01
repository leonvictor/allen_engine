#pragma once

#include <reflection/reflection.hpp>
#include <string>

#include "../editor_graph_node.hpp"

namespace aln
{
class AnimationClipEditorNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE()

  public:
    virtual void Serialize() override
    {
        // TODO
    }

    virtual void Initialize() override
    {
        m_name = "Animation Clip";
        AddOutputPin(PinValueType::Pose);
    }
};
} // namespace aln