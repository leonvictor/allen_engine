#include "assets/animation_graph/animation_graph_workspace.hpp"

#include "aln_imgui_widgets.hpp"

#include <entities/update_context.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <assert.h>

namespace aln
{
void AnimationGraphWorkspace::Compile()
{
    // TODO: Move the actual asset saving out of graph compilation
    m_rootGraph.Compile(m_compiledDefinitionPath, m_compiledDatasetPath, *m_pTypeRegistryService);
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
        else if (m_primaryGraphView.IsViewingStateMachine() && m_primaryGraphView.HasSelectedTransition()) // Selected nodes take precedence over links
        {
            auto pTransition = m_primaryGraphView.GetSelectedTransition();
            if (pTransition->HasChildGraph())
            {
                pSecondaryGraph = pTransition->GetChildGraph();
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

    auto HandleDoubleClick = [this](auto* pTransitionOrNode)
    { if (pTransitionOrNode->HasChildGraph())
        {
            auto pChildGraph = pTransitionOrNode->GetChildGraph();
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

    m_primaryGraphView.OnNodeDoubleClicked().BindListener(HandleDoubleClick);
    m_primaryGraphView.OnTransitionDoubleClicked().BindListener(HandleDoubleClick);
    m_primaryGraphView.OnCanvasDoubleClicked().BindListener(HandleCanvasDoubleClick);
    m_secondaryGraphView.OnNodeDoubleClicked().BindListener(HandleDoubleClick);
    m_secondaryGraphView.OnTransitionDoubleClicked().BindListener(HandleDoubleClick);
    m_secondaryGraphView.OnCanvasDoubleClicked().BindListener(HandleCanvasDoubleClick);

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

    Clear();
    IAssetWorkspace::Shutdown();
}

void AnimationGraphWorkspace::Clear()
{
    m_primaryGraphView.Clear();
    m_secondaryGraphView.Clear();
}
} // namespace aln