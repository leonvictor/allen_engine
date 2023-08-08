#pragma once

#include "editor_animation_graph.hpp"
#include "editor_animation_graph_node.hpp"

#include <anim/graph/graph_definition.hpp>
#include <anim/graph/runtime_graph_node.hpp>

#include <common/memory.hpp>

namespace aln
{

/// @brief The context passed around when compiling from an editor animation graph to its runtime version
class AnimationGraphCompilationContext
{
  private:
    const EditorAnimationGraph* m_pAnimationGraphEditor; // The graph currently compiling

    std::vector<const EditorAnimationGraphNode*> m_compiledNodes;
    std::vector<UUID> m_registeredDataSlots;

    size_t m_currentNodeMemoryOffset = 0;
    size_t m_maxNodeMemoryAlignement = 0;

  public:
    AnimationGraphCompilationContext(const EditorAnimationGraph* pAnimationGraphEditor)
        : m_pAnimationGraphEditor(pAnimationGraphEditor) {}

    const EditorAnimationGraphNode* GetNodeLinkedToInputPin(const UUID& inputPinID) const { return static_cast<const EditorAnimationGraphNode*>(m_pAnimationGraphEditor->GetNodeLinkedToInputPin(inputPinID)); }
    const EditorAnimationGraphNode* GetNodeLinkedToOutputPin(const UUID& outputPinID) const { return static_cast<const EditorAnimationGraphNode*>(m_pAnimationGraphEditor->GetNodeLinkedToOutputPin(outputPinID)); }

    /// @brief Try to get the runtime settings associated with an editor node
    /// @tparam T: Runtime type of the node
    /// @param pNode: Editor node
    /// @param pGraphDefinition: Definition of the graph this node is part of
    /// @param pSettings: Pointer to the node's settings storage
    /// @return Whether the node was already compiled (true: already compiled, false: not yet)
    template <typename T>
    bool GetSettings(const EditorAnimationGraphNode* pNode, AnimationGraphDefinition* pGraphDefinition, typename T::Settings*& pOutSettings)
    {
        static_assert(std::is_base_of_v<RuntimeGraphNode, T>);
        static_assert(std::is_base_of_v<RuntimeGraphNode::Settings, typename T::Settings>);

        auto it = std::find(m_compiledNodes.begin(), m_compiledNodes.end(), pNode);
        if (it != m_compiledNodes.end())
        {
            pOutSettings = (typename T::Settings*) pGraphDefinition->m_nodeSettings[it - m_compiledNodes.begin()];
            return true;
        }

        // Otherwise create the settings
        pOutSettings = aln::New<typename T::Settings>();
        pOutSettings->m_nodeIndex = m_compiledNodes.size();
        pGraphDefinition->m_nodeSettings.push_back(pOutSettings);
        pGraphDefinition->m_nodeIndices.push_back(pOutSettings->m_nodeIndex);

        // Update instance required memory info
        pGraphDefinition->m_nodeOffsets.push_back(m_currentNodeMemoryOffset);
        m_maxNodeMemoryAlignement = std::max(m_maxNodeMemoryAlignement, alignof(T));
        const auto requiredPadding = (alignof(T) - (m_currentNodeMemoryOffset % m_maxNodeMemoryAlignement)) % m_maxNodeMemoryAlignement;
        m_currentNodeMemoryOffset += (sizeof(T) + requiredPadding);

        m_compiledNodes.push_back(pNode);

        return false;
    }

    uint32_t RegisterDataSlot(const UUID& registeringNodeID)
    {
        auto id = m_registeredDataSlots.size();
        m_registeredDataSlots.push_back(registeringNodeID);
        return id;
    }

    const std::vector<UUID>& GetRegisteredDataSlots() const { return m_registeredDataSlots; }

    size_t GetNodeMemoryOffset() const { return m_currentNodeMemoryOffset; }
    size_t GetNodeMemoryAlignement() const { return m_maxNodeMemoryAlignement; }
};
} // namespace aln