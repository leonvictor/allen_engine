#pragma once

#include "animation_graph_dataset.hpp"
#include "graph_definition.hpp"
#include "runtime_graph_node.hpp"
#include "value_node.hpp"

#include <set>
#include <string>

namespace aln
{

/// @brief Represents an instance of the runtime state of a graph, associated with an animation dataset
class RuntimeGraphInstance
{
    /// @brief Definition that this instance refers to
    const AnimationGraphDefinition* m_pGraphDefinition = nullptr;

    // TODO: For now, this is an vector of nodes ptrs, and the actual data is *not* in contiguous memory
    // This should instead be a contiguous array, but its made more complicated by the fact node instances are not all the same size.
    // Its doable tho so do that !
    std::vector<RuntimeGraphNode*> m_runtimeNodeInstances;
    std::set<std::string, ValueNode*> m_controlParameters;


  public:
    RuntimeGraphInstance(AnimationGraphDefinition* pGraphDefinition) : m_pGraphDefinition(pGraphDefinition)
    {
        // TODO: Allocate a single block of memory for the whole graph (all nodes)
        // TODO: Runtime graph nodes are placement new'd into that block, and reference their settings via ptr (so settings are shared between instances of the same graph)
        // TODO: Runtime graph nodes are ordered by traversal order
        // TODO: See Node::CreateNode and replace by placement new

        m_runtimeNodeInstances.resize(m_pGraphDefinition->GetNumNodes());

        for (auto& pSettings : m_pGraphDefinition->m_nodeSettings)
        {
            pSettings->InstanciateNode(m_runtimeNodeInstances, m_pGraphDefinition->m_pDataset.get(), InitOptions::None);
        }

        // TODO ...
    }

    // Control parameters

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
};
} // namespace aln