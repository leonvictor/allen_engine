#pragma once

#include "graph/editor_graph_node.hpp"

#include <anim/types.hpp>

namespace aln
{

// fwd
class AnimationGraphDefinition;
class AnimationGraphCompilationContext;

class EditorAnimationGraphNode : public EditorGraphNode
{
    ALN_REGISTER_TYPE();

    friend class AnimationGraphWorkspace;

  public:
    /// @brief Compile the node and add it to a graph definition
    /// @param context Context for the running compilation
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const = 0;
};
} // namespace aln