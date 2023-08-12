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
    bool Compile(AnimationGraphDefinition& graphDefinition, AnimationGraphDataset& graphDataset, const TypeRegistryService& typeRegistryService, AnimationGraphCompilationContext& context);
};
} // namespace aln