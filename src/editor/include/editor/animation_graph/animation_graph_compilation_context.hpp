#pragma once

#include "animation_graph_editor.hpp"
#include "editor_graph_node.hpp"

#include <anim/graph/graph_definition.hpp>
#include <anim/graph/runtime_graph_node.hpp>

namespace aln
{

class AnimationGraphEditor;

/// @brief The context passed around when compiling from an editor animation graph to its runtime version
class AnimationGraphCompilationContext
{
  private:
    const AnimationGraphEditor* m_pAnimationGraphEditor; // The graph currently compiling

    // std::vector<RuntimeGraphNode::Settings*> m_nodeSettings;
    std::vector<const EditorGraphNode*> m_compiledNodes;
    std::vector<UUID> m_registeredDataSlots;

  public:
    AnimationGraphCompilationContext(const AnimationGraphEditor* pAnimationGraphEditor)
        : m_pAnimationGraphEditor(pAnimationGraphEditor) {}

    // const AnimationGraphEditor* GetAnimationGraphEditor() const { return m_pAnimationGraphEditor; }
    const EditorGraphNode* GetNodeLinkedToInputPin(const UUID& inputPinID) const { return m_pAnimationGraphEditor->GetNodeLinkedToInputPin(inputPinID); }
    const EditorGraphNode* GetNodeLinkedToOutputPin(const UUID& outputPinID) const { return m_pAnimationGraphEditor->GetNodeLinkedToOutputPin(outputPinID); }

    /// @brief Try to get the runtime settings associated with an editor node
    /// @tparam T: Runtime type of the node
    /// @param pNode: Editor node
    /// @param pGraphDefinition: Definition of the graph this node is part of
    /// @param pSettings: Pointer to the node's settings storage
    /// @return Whether the node was already compiled (true: already compiled, false: not yet)
    template <typename T>
    bool GetSettings(const EditorGraphNode* pNode, AnimationGraphDefinition* pGraphDefinition, typename T::Settings*& pOutSettings)
    {
        auto it = std::find(m_compiledNodes.begin(), m_compiledNodes.end(), pNode);
        if (it != m_compiledNodes.end())
        {
            pOutSettings = (typename T::Settings*) pGraphDefinition->m_nodeSettings[it - m_compiledNodes.begin()];
            return true;
        }

        // Otherwise create the settings
        pOutSettings = aln::New<T::Settings>();
        pOutSettings->m_nodeIndex = m_compiledNodes.size();
        pGraphDefinition->m_nodeSettings.push_back(pOutSettings);

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
};
} // namespace aln