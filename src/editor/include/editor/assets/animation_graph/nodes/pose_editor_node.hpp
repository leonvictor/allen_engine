#pragma once

#include <string>

#include <reflection/type_info.hpp>

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{
/// @brief Output node, receives the final pose to use in the frame
class PoseEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE();

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;
};
} // namespace aln