#include "assets/animation_graph/animation_graph_workspace.hpp"

#include "aln_imgui_widgets.hpp"
#include "assets/animation_graph/animation_graph_compilation_context.hpp"

#include <assets/asset_archive_header.hpp>
#include <entities/update_context.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <assert.h>

namespace aln
{
void AnimationGraphWorkspace::Compile()
{
    AnimationGraphCompilationContext context(&m_rootGraph);
    AnimationGraphDefinition graphDefinition;
    AnimationGraphDataset graphDataset;

    // TODO: Decouple dataset from the graph, use a dedicated dataset editor to associate anim clip node's user name to a given anim clip ID
    // TODO: Compile directly to this workspace's associated assets
    if (m_rootGraph.CompileDefinition(context, graphDefinition))
    {
        // TODO: Where does runtime asset serialization+saving occur ?
        // Serialize graph definition
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        reflect::TypeCollectionDescriptor typeCollectionDesc;
        for (auto& pSettings : graphDefinition.m_nodeSettings)
        {
            auto pSettingsTypeInfo = pSettings->GetTypeInfo();
            auto& descriptor = typeCollectionDesc.m_descriptors.emplace_back();
            descriptor.DescribeTypeInstance(pSettings, m_pTypeRegistryService, pSettingsTypeInfo);
        }

        dataArchive << typeCollectionDesc;
        dataArchive << graphDefinition;

        auto header = AssetArchiveHeader(AnimationGraphDefinition::GetStaticAssetTypeID());

        auto fileArchive = BinaryFileArchive(m_compiledDefinitionPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }
    else
    {
        // TODO: Log using a dedicated service
        std::cout << "Compilation error(s) occured: " << std::endl;
        for (auto& logEntry : context.GetErrorLog())
        {
            std::cout << logEntry.m_message << std::endl;
        }
    }

    if (m_rootGraph.CompileDataset(context, graphDataset))
    {
        // Serialize graph dataset
        std::vector<std::byte> data;
        auto dataArchive = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);
        graphDataset.Serialize(dataArchive);

        auto header = AssetArchiveHeader(AnimationGraphDataset::GetStaticAssetTypeID());
        for (auto& handle : graphDataset.m_animationClips)
        {
            header.AddDependency(handle.GetAssetID());
        }

        auto fileArchive = BinaryFileArchive(m_compiledDatasetPath, IBinaryArchive::IOMode::Write);
        fileArchive << header << data;
    }
    else
    {
        // TODO: Log using a dedicated service
        std::cout << "Compilation error(s) occured: " << std::endl;
        for (auto& logEntry : context.GetErrorLog())
        {
            std::cout << logEntry.m_message << std::endl;
        }
    }
}

void AnimationGraphWorkspace::SaveState(nlohmann::json& json) const
{
    m_rootGraph.SaveState(json["graph"]);
    m_primaryGraphView.SaveState(json["main_view"]);
    m_secondaryGraphView.SaveState(json["secondary_view"]);
}

void AnimationGraphWorkspace::LoadState(nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
{
    m_rootGraph.LoadState(json["graph"], pTypeRegistryService);
    m_primaryGraphView.LoadState(json["main_view"], pTypeRegistryService);
    m_secondaryGraphView.LoadState(json["secondary_view"], pTypeRegistryService);
}

void AnimationGraphWorkspace::Update(const UpdateContext& context)
{
    // TODO: It might be better to save that during initialization
    const auto pTypeRegistryService = context.GetService<TypeRegistryService>();

    std::string windowName = "Animation Graph";
    if (IsDirty())
    {
        // TODO: Modifying a window name changes its ID and thus it loses its position
        // windowName += "*";
    }

    if (m_waitingForAssetLoad)
    {
        if (m_pGraphDefinition.IsLoaded())
        {
            // TODO: Initialize the editor from loaded graph data
            m_waitingForAssetLoad = false;
        }
        else
        {
            // ImGui::Text("Waiting for asset load");
            // return; // TODO
        }
    }

    if (ImGui::Begin(windowName.c_str(), &m_isOpen))
    {
        auto contentRegion = ImGui::GetContentRegionAvail();

        if (m_firstUpdate)
        {
            m_primaryGraphViewHeight = contentRegion.y / 2;
            m_secondaryGraphViewHeight = contentRegion.y / 2;
            m_firstUpdate = false;
        }

        // TODO: Compile button shouldn't be necessary since all assets compilation will be done when exporting the game
        // This is a placeholder while we don't have a separation between game and scene editing
        if (ImGui::Button("Save & Compile"))
        {
            Compile();

            // TODO: Save to a common folder with the rest of the editor ?
            nlohmann::json json;
            SaveState(json);
            std::ofstream outputStream(m_statePath);
            outputStream << json;
        }

        // TMP : Account for the tempory compile button
        float primaryGraphViewCurrentHeight = contentRegion.y - ImGui::GetFrameHeightWithSpacing();
        // float primaryGraphViewCurrentHeight = contentRegion.y;
        // Hide secondary view if only one is required
        if (m_secondaryGraphView.HasGraphSet())
        {
            ImGuiWidgets::Splitter(false, 2, &m_primaryGraphViewHeight, &m_secondaryGraphViewHeight, 100, 100, contentRegion.x);
            primaryGraphViewCurrentHeight = m_primaryGraphViewHeight;
        }

        if (ImGui::BeginChild("Main Graph View", {contentRegion.x, primaryGraphViewCurrentHeight}, true))
        {
            m_primaryGraphView.Draw(pTypeRegistryService, m_graphDrawingContext);
            ImGui::EndChild();
        }

        EditorGraph* pSecondaryGraph = nullptr;
        if (m_primaryGraphView.IsMouseDragging())
        {
            pSecondaryGraph = m_secondaryGraphView.GetViewedGraph();
        }
        else if (m_primaryGraphView.HasSelectedNodes())
        {
            auto pLastSelectedNode = m_primaryGraphView.GetSelectedNodes().back();
            if (pLastSelectedNode->HasChildGraph())
            {
                pSecondaryGraph = pLastSelectedNode->GetChildGraph();
            }
        }
        else if (m_primaryGraphView.IsViewingStateMachine() && m_primaryGraphView.HasSelectedConduit()) // Selected nodes take precedence over links
        {
            auto pConduit = m_primaryGraphView.GetSelectedConduit();
            if (pConduit->HasChildGraph())
            {
                pSecondaryGraph = pConduit->GetChildGraph();
            }
        }
        m_secondaryGraphView.SetViewedGraph(pSecondaryGraph);

        if (m_secondaryGraphView.HasGraphSet() && ImGui::BeginChild("Secondary Graph View", {contentRegion.x, -1}, true))
        {
            m_secondaryGraphView.Draw(pTypeRegistryService, m_graphDrawingContext);
            ImGui::EndChild();
        }
    }
    ImGui::End();

    if (!m_isOpen)
    {
        // TODO: Ensure we've saved
        RequestAssetWindowDeletion(GetID());
    }
}

void AnimationGraphWorkspace::Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile)
{
    // TODO: Which parts of this could be shared behavior with a parent class ?
    IAssetWorkspace::Initialize(pContext, id);

    m_pTypeRegistryService = pContext->m_pTypeRegistryService;

    // TODO: Shared behavior with other asset windows ?
    m_compiledDefinitionPath = std::filesystem::path(id.GetAssetPath());

    m_compiledDatasetPath = m_compiledDefinitionPath;
    m_compiledDatasetPath.replace_extension(AnimationGraphDataset::GetStaticAssetTypeID().ToString());

    // Generate state path from id
    // TODO: state is saved in a separate directory
    m_statePath = m_compiledDefinitionPath;
    m_statePath.replace_filename(".json");

    auto HandleDoubleClick = [this](auto* pConduitOrNode)
    { if (pConduitOrNode->HasChildGraph())
        {
            auto pChildGraph = pConduitOrNode->GetChildGraph();
            if (m_primaryGraphView.GetViewedGraph() != pChildGraph)
            {
                m_primaryGraphView.SetViewedGraph(pChildGraph);
            }
    } };

    auto HandleCanvasDoubleClick = [this](auto* pEditorGraph)
    {
        if (pEditorGraph->HasParentGraph())
        {
            auto pParentGraph = pEditorGraph->GetParentGraph();
            if (m_primaryGraphView.GetViewedGraph() != pParentGraph)
            {
                m_primaryGraphView.SetViewedGraph(pParentGraph);
            }
        }
    };

    m_primaryGraphViewEventIDs.m_nodeDoubleClickedEventID = m_primaryGraphView.OnNodeDoubleClicked().BindListener(HandleDoubleClick);
    m_primaryGraphViewEventIDs.m_conduitDoubleClickedEventID = m_primaryGraphView.OnConduitDoubleClicked().BindListener(HandleDoubleClick);
    m_primaryGraphViewEventIDs.m_canvasDoubleClickedEventID = m_primaryGraphView.OnCanvasDoubleClicked().BindListener(HandleCanvasDoubleClick);

    m_secondaryGraphViewEventIDs.m_nodeDoubleClickedEventID = m_secondaryGraphView.OnNodeDoubleClicked().BindListener(HandleDoubleClick);
    m_secondaryGraphViewEventIDs.m_conduitDoubleClickedEventID = m_secondaryGraphView.OnConduitDoubleClicked().BindListener(HandleDoubleClick);
    m_secondaryGraphViewEventIDs.m_canvasDoubleClickedEventID = m_secondaryGraphView.OnCanvasDoubleClicked().BindListener(HandleCanvasDoubleClick);

    m_rootGraph.Initialize();
    m_primaryGraphView.SetViewedGraph(&m_rootGraph);

    if (readAssetFile)
    {
        if (std::filesystem::exists(m_statePath))
        {
            // TODO: Weird API
            std::ifstream inputStream(m_statePath);
            nlohmann::json json;
            inputStream >> json;

            LoadState(json, pContext->m_pTypeRegistryService);
        }
        else
        {
            // TODO: Load from compiled definition
        }
    }
}

void AnimationGraphWorkspace::Shutdown()
{
    // TODO
    if (IsDirty() && !m_statePath.empty())
    {
        nlohmann::json json;
        SaveState(json);
        std::ofstream outputStream(m_statePath);
        outputStream << json;
    }

    m_primaryGraphView.OnNodeDoubleClicked().UnbindListener(m_primaryGraphViewEventIDs.m_nodeDoubleClickedEventID);
    m_primaryGraphView.OnConduitDoubleClicked().UnbindListener(m_primaryGraphViewEventIDs.m_conduitDoubleClickedEventID);
    m_primaryGraphView.OnCanvasDoubleClicked().UnbindListener(m_primaryGraphViewEventIDs.m_canvasDoubleClickedEventID);

    m_secondaryGraphView.OnNodeDoubleClicked().UnbindListener(m_secondaryGraphViewEventIDs.m_nodeDoubleClickedEventID);
    m_secondaryGraphView.OnConduitDoubleClicked().UnbindListener(m_secondaryGraphViewEventIDs.m_conduitDoubleClickedEventID);
    m_secondaryGraphView.OnCanvasDoubleClicked().UnbindListener(m_secondaryGraphViewEventIDs.m_canvasDoubleClickedEventID);

    m_rootGraph.Shutdown();

    Clear();
    IAssetWorkspace::Shutdown();
}

void AnimationGraphWorkspace::Clear()
{
    m_primaryGraphView.Clear();
    m_secondaryGraphView.Clear();
}
} // namespace aln