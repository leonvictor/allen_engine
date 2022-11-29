#pragma once

#include "graph.hpp"

namespace aln
{
class RuntimeGraphDefinition
{
    // TODO:
    // Holds each node's settings in a contiguous array
    // TODO: How do i store various structs of different size in a contiguous way ?
    // Created from the editor only nodes via a compile step (== Instanciate() ?)

    static RuntimeGraphDefinition* Compile(Graph* pGraph)
    {
        // TODO
    }
};

class RuntimeGraphInstance
{
  private:
    RuntimeGraphInstance(RuntimeGraphDefinition* pGraph)
    {

        // TODO: Allocate a single block of memory for the whole graph (all nodes)
        // TODO: Runtime graph nodes are placement new'd into that block, and reference their settings via ptr (so settings are shared between instances of the same graph)
        // TODO: Runtime graph nodes are ordered by traversal order
    }

  public:
    static RuntimeGraphInstance* Instanciate(RuntimeGraphDefinition* pGraph)
    {
        // TODO: tmp
        return new RuntimeGraphInstance(pGraph);
    }
};
} // namespace aln