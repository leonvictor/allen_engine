#pragma once

#include <string>

#include <reflection/type_info.hpp>

#include "../editor_graph_node.hpp"

namespace aln
{
/// @brief Output node, receives the final pose to use in the frame
class PoseEditorNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE();

  public:
    virtual void Initialize() override;
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const override;
};
} // namespace aln