#pragma once

#include <assert.h>
#include <cinttypes>
#include <string>
#include <vector>

namespace aln
{
class Graph
{
    // TODO: Control parameters are value nodes that represent an instance of a value
    // Primary inputs to the graph
    // set once per frame before the graph evaluates
    // unique names per graph

    uint32_t GetParameterIndex(std::string parameterName)
    {
        // TODO
    }

    void SetParameter(uint32_t parameterIdx, float value)
    {
        // TODO
        // Use like this:
        // uint32_t const parameterIdx = graphInstance->GetParameterIndex("Speed");
        // graphInstance->SetParameter(parameterIdx, 5.0f);
    }

    void Evaluate()
    {
        // TODO
    }
};
} // namespace aln