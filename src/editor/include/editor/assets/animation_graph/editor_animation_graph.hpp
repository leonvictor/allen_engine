#pragma once

#include "graph/editor_graph.hpp"

#include <filesystem>

namespace aln
{

class AnimationGraphDefinition;

class EditorAnimationGraph : public EditorGraph
{
  public:
    // -------------- Asset compilation
    AnimationGraphDefinition* Compile(const std::filesystem::path& graphDefinitionPath, const std::filesystem::path& graphDatasetPath, const TypeRegistryService& typeRegistryService);
};
} // namespace aln