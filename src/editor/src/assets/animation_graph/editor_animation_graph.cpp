#include "assets/animation_graph/editor_animation_graph.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"
#include "assets/animation_graph/nodes/animation_clip_editor_node.hpp"
#include "assets/animation_graph/nodes/control_parameter_editor_nodes.hpp"
#include "assets/animation_graph/nodes/pose_editor_node.hpp"

#include <anim/graph/graph_definition.hpp>
#include <assets/asset_archive_header.hpp>
#include <common/containers/hash_map.hpp>
#include <common/serialization/binary_archive.hpp>

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(GRAPH_EDITORS, EditorAnimationGraph)
ALN_REFLECT_BASE(EditorGraph)
ALN_REGISTER_IMPL_END()

bool EditorAnimationGraph::CompileControlParameters(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    // Parameter nodes are compiled first to be easier to find
    assert(graphDefinition.m_controlParameterNames.empty());
    assert(graphDefinition.GetNumNodes() == 0);

    auto parameterNodes = GetAllNodesOfType<IControlParameterEditorNode>(NodeSearchScope::Recursive);
    graphDefinition.m_controlParameterNames.reserve(parameterNodes.size());
    for (auto& pParameterNode : parameterNodes)
    {
        auto parameterIdx = pParameterNode->Compile(context, graphDefinition);
        if (parameterIdx == InvalidIndex)
        {
            context.LogError("An error occured while compiling one of the graph's parameter nodes.", pParameterNode);
            return false;
        }

        graphDefinition.m_controlParameterNames.push_back(pParameterNode->GetName());
    }
    return true;
}

NodeIndex EditorAnimationGraph::CompileDefinition(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const
{
    auto pPreviousGraph = context.GetCurrentGraph();
    context.SetCurrentGraph(this);

    // ---- Compile graph definition

    auto outputNodes = GetAllNodesOfType<PoseEditorNode>(NodeSearchScope::Local);
    // TODO: Identify the graph by a user-readable name
    if (outputNodes.size() < 1)
    {
        context.LogError("No result pose node found.");
        context.SetCurrentGraph(pPreviousGraph);
        return InvalidIndex;
    }
    if (outputNodes.size() > 1)
    {
        context.LogError("More than one result pose node found.");
        context.SetCurrentGraph(pPreviousGraph);
        return InvalidIndex;
    }

    auto rootNodeIndex = outputNodes[0]->Compile(context, graphDefinition);
    // TODO: Record an error if necessary, them read them back in the reverse order as a stack trace

    context.SetCurrentGraph(pPreviousGraph);

    return rootNodeIndex;
}

bool EditorAnimationGraph::CompileDataset(AnimationGraphCompilationContext& context, AnimationGraphDataset& graphDataset) const
{
    const auto animationClipNodes = GetAllNodesOfType<AnimationClipEditorNode>(NodeSearchScope::Recursive);

    HashMap<UUID, const AnimationClipEditorNode*> animationClipNodeLookupMap;
    for (const auto pNode : animationClipNodes)
    {
        animationClipNodeLookupMap[pNode->GetID()] = pNode;
    }

    auto& registeredDataSlots = context.GetRegisteredDataSlots();
    graphDataset.m_animationClips.reserve(registeredDataSlots.size());
    for (auto& slotOwnerNodeID : registeredDataSlots)
    {
        const auto pOwnerNode = animationClipNodeLookupMap[slotOwnerNodeID];
        const auto& clipID = pOwnerNode->GetAnimationClipID();

        if (!clipID.IsValid())
        {
            context.LogError("Wrong animation clip ID provided to AnimationClip node.", pOwnerNode);
            return false;
        }

        graphDataset.m_animationClips.emplace_back(clipID);
    }
    return true;
}
} // namespace aln