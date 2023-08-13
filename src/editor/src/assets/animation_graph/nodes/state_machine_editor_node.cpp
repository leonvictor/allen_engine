#include "assets/animation_graph/nodes/state_machine_editor_node.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"
#include "assets/animation_graph/editor_animation_state_machine.hpp"

#include <anim/graph/nodes/state_machine.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(ANIM_GRAPH_EDITOR_NODES, StateMachineEditorNode)
ALN_REGISTER_IMPL_END()

void StateMachineEditorNode::Initialize()
{
    m_name = "State Machine";
    AddOutputPin(NodeValueType::Pose, "Result");

    EditorAnimationStateMachine* pEditorStateMachine = aln::New<EditorAnimationStateMachine>();
    SetChildGraph(pEditorStateMachine);
}

NodeIndex StateMachineEditorNode::Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    StateMachineRuntimeNode::Settings* pStateMachineNodeSettings = nullptr;
    bool compiled = context.GetSettings<StateMachineRuntimeNode>(this, graphDefinition, pStateMachineNodeSettings);
    if (!compiled)
    {
        auto pStateMachine = static_cast<EditorAnimationStateMachine*>(GetChildGraph());

        std::unordered_map<UUID, NodeIndex> stateIDToCompiledNodeIndexMapping; // Indices of the state node in the compiled node array
        std::unordered_map<UUID, uint32_t> stateIDToStateSettingsIndexMapping; // Indices of the state's settings in the state machine's state settings array

        // Compile states
        auto stateNodes = pStateMachine->GetAllNodesOfType<StateEditorNode>();

        auto stateNodesCount = stateNodes.size();
        if (stateNodesCount < 1)
        {
            context.LogError("A state machine graph must hold at least one state node.");
            return InvalidIndex;
        }

        for (const auto pStateNode : stateNodes)
        {
            const auto stateSettingsIndex = pStateMachineNodeSettings->m_stateSettings.size();

            auto& stateSettings = pStateMachineNodeSettings->m_stateSettings.emplace_back();
            stateSettings.m_stateNodeIndex = pStateNode->Compile(context, graphDefinition);
            if (stateSettings.m_stateNodeIndex == InvalidIndex)
            {
                context.LogError("Error during state node compilation.", pStateNode);
                return InvalidIndex;
            }

            stateIDToCompiledNodeIndexMapping[pStateNode->GetID()] = stateSettings.m_stateNodeIndex;
            stateIDToStateSettingsIndexMapping[pStateNode->GetID()] = stateSettingsIndex;
        }

        // Compile conduits
        const auto& conduits = pStateMachine->GetConduits();
        for (auto startStateNodeIdx = 0; startStateNodeIdx < stateNodesCount; ++startStateNodeIdx)
        {
            const auto pStartStateNode = stateNodes[startStateNodeIdx];
            for (const auto& pConduit : conduits)
            {
                // Compile conduits matching the start state
                if (pConduit->GetStartState() == pStartStateNode)
                {
                    const auto pPreviousGraph = context.GetCurrentGraph();

                    // Compile the child graph, which contains the actual transitions
                    const auto pConduitGraph = pConduit->GetChildGraph();
                    context.SetCurrentGraph(pConduitGraph);

                    const auto pEndStateNode = pConduit->GetEndState();
                    const auto endStateSettingsIdx = stateIDToStateSettingsIndexMapping.at(pEndStateNode->GetID());
                    const auto endStateNodeIdx = stateIDToCompiledNodeIndexMapping.at(pEndStateNode->GetID());

                    const auto& transitionNodes = pConduitGraph->GetAllNodesOfType<TransitionEditorNode>();
                    if (transitionNodes.size() < 1)
                    {
                        // TODO: Empty conduits do not fail, they are used to override and disable an existing global transition
                        context.LogError("A conduit must hold at least one transition.");
                        context.SetCurrentGraph(pPreviousGraph);
                        return InvalidIndex; // TODO: Do not immediately return here, and try to compile the rest of the conduits before failing
                    }

                    // Actually compile the transitions
                    for (const auto pTransitionNode : transitionNodes)
                    {
                        const auto transitionNodeIndex = CompileTransition(context, graphDefinition, pTransitionNode, endStateNodeIdx);
                        if (transitionNodeIndex == InvalidIndex)
                        {
                            context.LogError("An error occured while compiling a conduit's transition node.", pTransitionNode);
                            context.SetCurrentGraph(pPreviousGraph);
                            return InvalidIndex;
                        }

                        // Compile the condition node (afterwards because its index is held in the state machine node's settings)
                        const auto pConditionNode = context.GetNodeLinkedToInputPin(pTransitionNode->GetInputPin(0).GetID());
                        if (pConditionNode == nullptr)
                        {
                            // TODO: Having no condition node set is not a fatal error and simply disables the transition
                            context.LogError("No condition node set.", this);
                            context.SetCurrentGraph(pPreviousGraph);
                            return InvalidIndex;
                        }

                        const auto conditionNodeIndex = pConditionNode->Compile(context, graphDefinition);
                        if (conditionNodeIndex == InvalidIndex)
                        {
                            context.LogError("An error occured while trying to compile the condition node.", pConditionNode);
                            context.SetCurrentGraph(pPreviousGraph);
                            return InvalidIndex;
                        }

                        // Set the state machine node's settings
                        auto& transitionSettings = pStateMachineNodeSettings->m_stateSettings[startStateNodeIdx].m_transitionSettings.emplace_back();
                        transitionSettings.m_targetStateIndex = endStateSettingsIdx;
                        transitionSettings.m_transitionNodeIndex = transitionNodeIndex;
                        transitionSettings.m_conditionNodeIndex = conditionNodeIndex;
                    }

                    context.SetCurrentGraph(pPreviousGraph);
                }
            }
        }
    }
    return pStateMachineNodeSettings->GetNodeIndex();
};

NodeIndex StateMachineEditorNode::CompileTransition(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition, const TransitionEditorNode* pTransitionNode, NodeIndex endStateNodeIdx) const
{
    TransitionRuntimeNode::Settings* pTransitionNodeSettings = nullptr;
    bool compiled = context.GetSettings<TransitionRuntimeNode>(this, graphDefinition, pTransitionNodeSettings);
    if (!compiled)
    {
        // Set the transition node's runtime settings
        pTransitionNodeSettings->m_duration = pTransitionNode->m_duration;
        pTransitionNodeSettings->m_targetStateNodeIdx = endStateNodeIdx;
    }
    return pTransitionNodeSettings->GetNodeIndex();
}

} // namespace aln
