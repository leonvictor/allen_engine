#include "assets/animation_graph/editor_animation_graph.hpp"

#include "assets/animation_graph/animation_graph_compilation_context.hpp"
#include "assets/animation_graph/nodes/animation_clip_editor_node.hpp"
#include "assets/animation_graph/nodes/control_parameter_editor_nodes.hpp"
#include "assets/animation_graph/nodes/pose_editor_node.hpp"

#include <anim/graph/graph_definition.hpp>
#include <common/serialization/binary_archive.hpp>
#include <assets/asset_archive_header.hpp>

namespace aln
{

 // TODO: Move the actual serialization out 
AnimationGraphDefinition* EditorAnimationGraph::Compile(const std::filesystem::path& graphDefinitionPath, const std::filesystem::path& graphDatasetPath, const TypeRegistryService& typeRegistryService)
{
    AnimationGraphCompilationContext context(this);

    // Compile graph definition
    AnimationGraphDefinition graphDefinition;

    // Parameter nodes are compiled first to be easier to find
    auto parameterNodes = GetAllNodesOfType<IControlParameterEditorNode>();
    graphDefinition.m_controlParameterNames.reserve(parameterNodes.size());
    for (auto& pParameterNode : parameterNodes)
    {
        pParameterNode->Compile(context, &graphDefinition);
        graphDefinition.m_controlParameterNames.push_back(pParameterNode->GetName());
    }

    auto outputNodes = GetAllNodesOfType<PoseEditorNode>();
    assert(outputNodes.size() == 1);

    auto rootNodeIndex = outputNodes[0]->Compile(context, &graphDefinition);
    graphDefinition.m_rootNodeIndex = rootNodeIndex;
    graphDefinition.m_requiredMemoryAlignement = context.GetNodeMemoryAlignement();
    graphDefinition.m_requiredMemorySize = context.GetNodeMemoryOffset();

    // Compile dataset
    AnimationGraphDataset graphDataset;
    auto& registeredDataSlots = context.GetRegisteredDataSlots();
    for (auto& slotOwnerNodeID : registeredDataSlots)
    {
        const auto pOwnerNode = (AnimationClipEditorNode*) GetNode(slotOwnerNodeID);
        auto& clipID = pOwnerNode->GetAnimationClipID();
        assert(clipID.IsValid()); // TODO: Proper input validation
        graphDataset.m_animationClips.emplace_back(clipID);
    }

    // TODO: Where does runtime asset serialization+saving occur ?

    // Serialize graph definition
    {
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        reflect::TypeCollectionDescriptor typeCollectionDesc;
        for (auto& pSettings : graphDefinition.m_nodeSettings)
        {
            auto pSettingsTypeInfo = pSettings->GetTypeInfo();
            auto& descriptor = typeCollectionDesc.m_descriptors.emplace_back();
            descriptor.DescribeTypeInstance(pSettings, &typeRegistryService, pSettingsTypeInfo);
        }

        dataArchive << typeCollectionDesc;
        dataArchive << graphDefinition;

        auto header = AssetArchiveHeader(AnimationGraphDefinition::GetStaticAssetTypeID());

        auto fileArchive = BinaryFileArchive(graphDefinitionPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }

    // Serialize graph dataset
    {
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);
        graphDataset.Serialize(dataArchive);

        auto header = AssetArchiveHeader(AnimationGraphDataset::GetStaticAssetTypeID());
        for (auto& handle : graphDataset.m_animationClips)
        {
            header.AddDependency(handle.GetAssetID());
        }

        auto fileArchive = BinaryFileArchive(graphDatasetPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }
    return nullptr;
}
} // namespace aln