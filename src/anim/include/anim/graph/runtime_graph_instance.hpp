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
class RuntimeAnimationGraphInstance
{
    friend class AnimationGraphComponent;

    const AnimationGraphDefinition* m_pGraphDefinition = nullptr;
    const AnimationGraphDataset* m_pGraphDataset = nullptr;

    std::byte* m_pNodeInstancesMemory = nullptr;

    std::vector<RuntimeGraphNode*> m_runtimeNodeInstances;
    std::set<std::string, ValueNode*> m_controlParameters;

  public:
    RuntimeAnimationGraphInstance(const AnimationGraphDefinition* pGraphDefinition, const AnimationGraphDataset* pGraphDataset)
        : m_pGraphDefinition(pGraphDefinition), m_pGraphDataset(pGraphDataset)
    {
        // Allocate a single block of memory for the whole graph
        m_pNodeInstancesMemory = (std::byte*) aln::Allocate(pGraphDefinition->m_requiredMemorySize, pGraphDefinition->m_requiredMemoryAlignement);
        m_runtimeNodeInstances.reserve(m_pGraphDefinition->GetNumNodes());
        for (auto& nodeOffset : pGraphDefinition->m_nodeOffsets)
        {
            m_runtimeNodeInstances.emplace_back(reinterpret_cast<RuntimeGraphNode*>(m_pNodeInstancesMemory + nodeOffset));
        }

        // Instanciate each node in the allocated memory
        for (auto& pSettings : m_pGraphDefinition->m_nodeSettings)
        {
            pSettings->InstanciateNode(m_runtimeNodeInstances, m_pGraphDataset, InitOptions::None);
        }
    }

    ~RuntimeAnimationGraphInstance()
    {
        for (auto& pNode : m_runtimeNodeInstances)
        {
            pNode->~RuntimeGraphNode();
        }
        aln::Free(m_pNodeInstancesMemory);
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