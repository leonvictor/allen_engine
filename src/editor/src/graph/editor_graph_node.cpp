#include "graph/editor_graph_node.hpp"

#include "graph/editor_graph.hpp"

#include <common/maths/maths.hpp>
#include <common/serialization/json.hpp>
#include <common/types.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imnodes.h>

namespace aln
{
ALN_REGISTER_ABSTRACT_IMPL_BEGIN(EditorGraphNode);
ALN_REGISTER_IMPL_END();

float EditorGraphNode::CalcNodeWidth() const
{
    constexpr float MIN_NODE_WIDTH = 120.0f;

    float nodeWidth = Maths::Max(MIN_NODE_WIDTH, ImGui::CalcTextSize(GetName().c_str()).x);

    for (const auto& pin : GetInputPins())
    {
        nodeWidth = Maths::Max(nodeWidth, ImGui::CalcTextSize(pin.GetName().c_str()).x);
    }

    for (const auto& pin : GetOutputPins())
    {
        nodeWidth = Maths::Max(nodeWidth, ImGui::CalcTextSize(pin.GetName().c_str()).x);
    }

    const auto pTypeInfo = GetTypeInfo();
    for (const auto& member : pTypeInfo->m_members)
    {
        nodeWidth = Maths::Max(nodeWidth, ImGui::CalcTextSize(member.GetPrettyName().c_str()).x + 100);
    }

    return nodeWidth;
}

void EditorGraphNode::PushNodeStyle(const GraphDrawingContext& ctx) const
{
    auto colorScheme = ctx.GetTypeColorScheme(GetValueType());

    ImNodes::PushColorStyle(ImNodesCol_TitleBar, static_cast<uint32_t>(colorScheme.m_defaultColor));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, static_cast<uint32_t>(colorScheme.m_hoveredColor));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, static_cast<uint32_t>(colorScheme.m_selectedColor));
}

void EditorGraphNode::PopNodeStyle(const GraphDrawingContext& ctx) const
{
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
}

const Pin& EditorGraphNode::AddInputPin(NodeValueType valueType, std::string name)
{
    auto& pin = m_inputPins.emplace_back();
    pin.m_type = Pin::Type::In;
    pin.m_valueType = valueType;
    pin.m_name = name;
    pin.m_allowMultipleLinks = false;

    return pin;
}

const Pin& EditorGraphNode::AddOutputPin(NodeValueType valueType, std::string name, bool allowMultipleLinks)
{
    auto& pin = m_outputPins.emplace_back();
    pin.m_type = Pin::Type::Out;
    pin.m_valueType = valueType;
    pin.m_name = name;
    pin.m_allowMultipleLinks = allowMultipleLinks;

    return pin;
}

const Pin& EditorGraphNode::AddDynamicInputPin(NodeValueType valueType, std::string name)
{
    AddInputPin(valueType, name);

    auto& pin = m_inputPins.back();
    pin.m_dynamic = true;

    return pin;
}

void EditorGraphNode::RemoveDynamicInputPin(const UUID& pinID)
{
    assert(pinID.IsValid());

    OnDynamicInputPinRemoved(pinID);

    auto it = std::find_if(m_inputPins.begin(), m_inputPins.end(), [&](const Pin& pin)
        { return pin.GetID() == pinID; });
    assert(it != m_inputPins.end());

    m_inputPins.erase(it);
}

void EditorGraphNode::DrawNodeTitleBar(const GraphDrawingContext& ctx)
{
    ImNodes::BeginNodeTitleBar();

    if (IsRenamable())
    {
        if (m_renamingInProgress)
        {
            ImGui::PushItemWidth(ctx.m_currentNodeWidth);
            ImGui::InputText("", &m_name);
            ImGui::PopItemWidth();

            if (m_renamingStarted)
            {
                ImGui::SetKeyboardFocusHere(-1);
                m_renamingStarted = false;
            }
            if (ImGui::IsItemDeactivated())
            {
                EndRenaming();
            }
        }
        else
        {
            ImGui::Text(GetName().c_str());
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                BeginRenaming();
            }
        }
    }
    else
    {
        ImGui::Text(GetName().c_str());
    }

    ImNodes::EndNodeTitleBar();
}

bool EditorGraphNode::DrawContent(const GraphDrawingContext& ctx)
{
    auto pTypeInfo = GetTypeInfo();
    ctx.m_nodeInspector.Draw(pTypeInfo, (void*) this, 100);
    return pTypeInfo->GetMemberCount() > 0;
}

bool EditorGraphNode::DrawPin(const Pin& pin, const GraphDrawingContext& ctx)
{
    auto colorScheme = ctx.GetTypeColorScheme(pin.GetValueType());
    ImNodes::PushColorStyle(ImNodesCol_Pin, static_cast<uint32_t>(colorScheme.m_defaultColor));
    ImNodes::PushColorStyle(ImNodesCol_PinHovered, static_cast<uint32_t>(colorScheme.m_hoveredColor));

    if (pin.IsInput())
    {
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
        ImNodes::BeginInputAttribute(pin.GetID());

        const auto pPinName = pin.GetName().c_str();
        const auto offset = ctx.m_currentNodeWidth - ImGui::CalcTextSize(pPinName).x;
        ImGui::Text(pPinName);
        ImGui::SameLine();
        ImGui::Dummy({offset, 0.0f});

        ImNodes::EndInputAttribute();
        ImNodes::PopAttributeFlag();
    }
    else
    {
        const auto pinFlags = pin.AllowsMultipleLinks() ? ImNodesAttributeFlags_None : ImNodesAttributeFlags_EnableLinkDetachWithDragClick;
        ImNodes::PushAttributeFlag(pinFlags);
        ImNodes::BeginOutputAttribute(pin.GetID());

        const char* pinName = pin.GetName().c_str();

        /// @note : I'd like to align output pin labels to the right side of the node.
        /// However, for now, the node's dimensions are only known after the ImNodes::EndNode() call
        /// The following attempt result in forever growing node width:
        const float labelWidth = ImGui::CalcTextSize(pinName).x;
        // const float nodeWidth = ImNodes::GetNodeDimensions(GetID()).x;
        auto offset = ctx.m_currentNodeWidth - labelWidth;
        ImGui::Indent(offset);

        ImGui::Text(pinName);

        ImNodes::EndOutputAttribute();
        ImNodes::PopAttributeFlag();
    }

    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();

    return true;
}

EditorGraphNode::~EditorGraphNode()
{
    assert(m_pChildGraph == nullptr);
}

NodeValueType EditorGraphNode::GetValueType() const
{
    if (m_outputPins.size() > 0)
    {
        return GetOutputPin(0).m_valueType;
    }
    else if (m_inputPins.size() > 0)
    {
        return GetInputPin(0).m_valueType;
    }
    else
    {
        return NodeValueType::Unknown;
    }
}

uint32_t EditorGraphNode::GetInputPinIndex(const UUID& pinID) const
{
    uint32_t pinCount = m_inputPins.size();
    for (uint32_t pinIndex = 0; pinIndex < pinCount; ++pinIndex)
    {
        auto& pin = m_inputPins[pinIndex];
        if (pin.GetID() == pinID)
        {
            return pinIndex;
        }
    }
    return InvalidIndex;
}

uint32_t EditorGraphNode::GetOutputPinIndex(const UUID& pinID) const
{
    uint32_t pinCount = m_outputPins.size();
    for (uint32_t pinIndex = 0; pinIndex < pinCount; ++pinIndex)
    {
        auto& pin = m_outputPins[pinIndex];
        if (pin.GetID() == pinID)
        {
            return pinIndex;
        }
    }
    return InvalidIndex;
}

void EditorGraphNode::SetChildGraph(EditorGraph* pChildGraph)
{
    assert(pChildGraph != nullptr && m_pChildGraph == nullptr);
    assert(!pChildGraph->IsInitialized());

    pChildGraph->Initialize();

    m_pChildGraph = pChildGraph;
}

void EditorGraphNode::Shutdown()
{
    if (m_pChildGraph != nullptr)
    {
        m_pChildGraph->Shutdown();
        aln::Delete(m_pChildGraph);
        m_pChildGraph = nullptr;
    }
}

void EditorGraphNode::SaveNodeState(JSON& json) const
{
    json["name"] = m_name;

    if (!m_inputPins.empty())
    {
        auto& inputPinsJson = json["input_pins"];
        for (auto& pin : m_inputPins)
        {
            auto& pinJson = inputPinsJson.emplace_back();
            pinJson["name"] = pin.m_name;
            pinJson["value_type"] = pin.m_valueType;
            pinJson["allows_multiple_links"] = pin.m_allowMultipleLinks;
            pinJson["dynamic"] = pin.m_dynamic;
        }
    }

    if (!m_outputPins.empty())
    {
        auto& outputPinsJson = json["output_pins"];
        for (auto& pin : m_outputPins)
        {
            auto& pinJson = outputPinsJson.emplace_back();
            pinJson["name"] = pin.m_name;
            pinJson["value_type"] = pin.m_valueType;
            pinJson["allows_multiple_links"] = pin.m_allowMultipleLinks;
            pinJson["dynamic"] = pin.m_dynamic;
        }
    }

    SaveState(json);
}

void EditorGraphNode::LoadNodeState(const JSON& json, const TypeRegistryService* pTypeRegistryService)
{
    m_name = json["name"];

    if (json.contains("input_pins"))
    {
        auto& inputPinsJson = json["input_pins"];
        for (auto& pinJson : inputPinsJson)
        {
            auto& pin = m_inputPins.emplace_back();
            pin.m_name = pinJson["name"];
            pin.m_valueType = pinJson["value_type"];
            pin.m_allowMultipleLinks = pinJson["allows_multiple_links"];
            pin.m_dynamic = pinJson["dynamic"];
            pin.m_type = Pin::Type::In;
        }
    }

    if (json.contains("output_pins"))
    {
        auto& outputPinsJson = json["output_pins"];
        for (auto& pinJson : outputPinsJson)
        {
            auto& pin = m_outputPins.emplace_back();
            pin.m_name = pinJson["name"];
            pin.m_valueType = pinJson["value_type"];
            pin.m_allowMultipleLinks = pinJson["allows_multiple_links"];
            pin.m_dynamic = pinJson["dynamic"];
            pin.m_type = Pin::Type::Out;
        }
    }

    LoadState(json, pTypeRegistryService);
}

void EditorGraphNode::DrawNode(GraphDrawingContext& ctx)
{
    ctx.m_currentNodeWidth = CalcNodeWidth();

    PushNodeStyle(ctx);
    ImNodes::BeginNode(GetID());

    DrawNodeTitleBar(ctx);

    for (auto& inputPin : m_inputPins)
    {
        DrawPin(inputPin, ctx);
    }

    // Display reflected fields
    DrawContent(ctx);

    for (auto& outputPin : m_outputPins)
    {
        DrawPin(outputPin, ctx);
    }

    ImNodes::EndNode();
    PopNodeStyle(ctx);
}

} // namespace aln