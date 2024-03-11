#pragma once

#include "graph/editor_graph.hpp"

#include <anim/types.hpp>

namespace aln
{

class AnimationGraphDefinition;
class AnimationGraphDataset;
class AnimationGraphCompilationContext;

class EditorAnimationGraph : public EditorGraph
{
    ALN_REGISTER_TYPE()

  public:
    // -------------- Asset compilation
    bool CompileControlParameters(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const;
    NodeIndex CompileDefinition(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const;
    bool CompileDataset(AnimationGraphCompilationContext& context, AnimationGraphDataset& graphDataset) const;
};
} // namespace aln