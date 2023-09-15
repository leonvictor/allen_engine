#pragma once

#include "animation_graph_dataset.hpp"
#include "graph_definition.hpp"
#include "pose_node.hpp"
#include "runtime_graph_node.hpp"
#include "value_node.hpp"

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

    Vector<RuntimeGraphNode*> m_runtimeNodeInstances;

    PoseRuntimeNode* m_pRootNode = nullptr;

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

    void Initialize(GraphContext& context)
    {
        for (auto& pNode : m_runtimeNodeInstances)
        {
            pNode->Initialize(context);
        }

        m_pRootNode = static_cast<PoseRuntimeNode*>(m_runtimeNodeInstances[m_pGraphDefinition->m_rootNodeIndex]);
    }

    void Shutdown()
    {
        m_pRootNode = nullptr;

        for (auto pNode : m_runtimeNodeInstances)
        {
            pNode->Shutdown();
        }
    }

    bool IsInitialized() const { return m_pRootNode != nullptr && m_pRootNode->IsInitialized(); }

    PoseNodeResult Update(GraphContext& context)
    {
        assert(IsInitialized());
        return m_pRootNode->Update(context);
    }

    // ----- Control parameters

    size_t GetControlParameterCount() const { return m_pGraphDefinition->m_controlParameterNames.size(); }

    // TODO: set once per frame before the graph evaluates
    // TODO: unique names per graph
    NodeIndex GetControlParameterIndex(const StringID& parameterName) const
    {
        auto parameterCount = GetControlParameterCount();
        for (auto parameterIdx = 0; parameterIdx < parameterCount; ++parameterIdx)
        {
            if (m_pGraphDefinition->m_controlParameterNames[parameterIdx] == parameterName)
            {
                return parameterIdx;
            }
        }
        return InvalidIndex;
    }

    template <typename T>
    void SetControlParameterValue(GraphContext& context, NodeIndex parameterIdx, T value)
    {
        assert(parameterIdx < GetControlParameterCount());
        auto pParameterNode = reinterpret_cast<ValueNode*>(m_runtimeNodeInstances[parameterIdx]);
        pParameterNode->SetValue<T>(context, value);
    }

    template <typename T>
    const T& GetControlParameterValue(GraphContext& context, NodeIndex parameterIdx)
    {
        assert(parameterIdx < GetControlParameterCount());
        auto pParameterNode = reinterpret_cast<ValueNode*>(m_runtimeNodeInstances[parameterIdx]);
        return pParameterNode->GetValue<T>(context);
    }
};
} // namespace aln