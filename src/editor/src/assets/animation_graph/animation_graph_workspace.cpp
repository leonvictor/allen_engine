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
            //return; // TODO
        }
    }

    if (ImGui::Begin(windowName.c_str(), &m_isOpen))
    {
        auto contentRegion = ImGui::GetContentRegionAvail();
        static float topSectionHeight = contentRegion.y / 2;
        static float bottomSectionHeight = contentRegion.y / 2;
        ImGuiWidgets::Splitter(false, 2, &topSectionHeight, &bottomSectionHeight, 200, 200, contentRegion.x);

        if (ImGui::BeginChild("Main Graph View", {contentRegion.x, topSectionHeight}, true))
        {
            //ImGui::Text("first");
            m_primaryGraphView.Draw(pTypeRegistryService, m_graphDrawingContext);
           
            ImGui::EndChild();
        }

        if (ImGui::BeginChild("Secondary Graph View", {contentRegion.x, -1}, true))
        {
            //ImGui::Text("second");
            //m_secondaryGraphView.Update(context);
            
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