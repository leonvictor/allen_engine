#pragma once

#include "graph/editor_graph.hpp"

namespace aln
{

class AnimationGraphDefinition;
class AnimationGraphDataset;
class AnimationGraphCompilationContext;

/// @todo Rename to Blend Tree
class EditorAnimationGraph : public EditorGraph
{
  public:
    // -------------- Asset compilation
    bool CompileDefinition(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const;
    bool CompileDataset(AnimationGraphCompilationContext& context, AnimationGraphDataset& graphDataset) const;
};
} // namespace aln