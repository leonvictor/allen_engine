#pragma once

#include "pin.hpp"

#include <animation_graph/animation_graph_compilation_context.hpp>
#include <common/types.hpp>
#include <common/uuid.hpp>
#include <reflection/reflected_type.hpp>
#include <reflection/type_info.hpp>

#include <imgui_stdlib.h>
#include <nlohmann/json.hpp>

#include <cmath>

namespace aln
{

// fwd
class AnimationGraphDefinition;
class AnimationGraphCompilationContext;

class EditorGraphNode : public reflect::IReflected
{
    ALN_REGISTER_TYPE();

    friend class AnimationGraphEditor;

  private:
    const UUID m_id = UUID::Generate();
    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;

    // TODO: This is an awful lot of state
    bool m_renamingStarted = false;
    bool m_renamingInProgress = false;

    float CalcNodeWidth() const
    {
        constexpr float MIN_NODE_WIDTH = 120.0f;

        float nodeWidth = std::max(MIN_NODE_WIDTH, ImGui::CalcTextSize(GetName().c_str()).x);

        for (const auto& pin : GetInputPins())
        {
            nodeWidth = std::max(nodeWidth, ImGui::CalcTextSize(pin.GetName().c_str()).x);
        }

        for (const auto& pin : GetOutputPins())
        {
            nodeWidth = std::max(nodeWidth, ImGui::CalcTextSize(pin.GetName().c_str()).x);
        }

        const auto pTypeInfo = GetTypeInfo();
        for (const auto& member : pTypeInfo->m_members)
        {
            nodeWidth = std::max(nodeWidth, ImGui::CalcTextSize(member.GetPrettyName().c_str()).x + 100);
        }

        return nodeWidth;
    }

    // TODO: Dynamic output

  protected:
    std::string m_name;
    bool m_renamable = false;

    const Pin& AddInputPin(NodeValueType valueType, std::string name = "")
    {
        auto& pin = m_inputPins.emplace_back();
        pin.m_type = Pin::Type::In;
        pin.m_valueType = valueType;
        pin.m_name = name;
        pin.m_allowMultipleLinks = false;

        return pin;
    }

    const Pin& AddOutputPin(NodeValueType valueType, std::string name = "", bool allowMultipleLinks = false)
    {
        auto& pin = m_outputPins.emplace_back();
        pin.m_type = Pin::Type::Out;
        pin.m_valueType = valueType;
        pin.m_name = name;
        pin.m_allowMultipleLinks = allowMultipleLinks;

        return pin;
    }

    const Pin& AddDynamicInputPin(NodeValueType valueType, std::string name = "")
    {
        AddInputPin(valueType, name);

        auto& pin = m_inputPins.back();
        pin.m_dynamic = true;

        return pin;
    }

    void RemoveDynamicInputPin(const UUID& pinID)
    {
        assert(pinID.IsValid());

        OnDynamicInputPinRemoved(pinID);

        auto it = std::find_if(m_inputPins.begin(), m_inputPins.end(), [&](const Pin& pin)
            { return pin.GetID() == pinID; });
        assert(it != m_inputPins.end());

        m_inputPins.erase(it);
    }

    // ---- Dynamic pins creation/deletion callbacks
    virtual void OnDynamicInputPinCreated(const UUID& pinID) {}
    virtual void OnDynamicInputPinRemoved(const UUID& pinID) {}
    virtual void OnDynamicOutputPinCreated(const UUID& pinID) {}
    virtual void OnDynamicOuputPinRemoved(const UUID& pinID) {}

    // ---- Custom drawing
    virtual void DrawNodeTitleBar(const GraphDrawingContext& ctx)
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

    virtual bool DrawContent(const GraphDrawingContext& ctx)
    {
        auto pTypeInfo = GetTypeInfo();
        ctx.m_nodeInspector.Draw(pTypeInfo, (void*) this, 100);
        return pTypeInfo->GetMemberCount() > 0;
    }

    virtual bool DrawPin(const Pin& pin, const GraphDrawingContext& ctx)
    {
        ImNodes::PushColorStyle(ImNodesCol_Pin, ctx.GetTypeColor(pin.GetValueType()).U32());
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

        return true;
    }

    // ---- Custom serialization
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) {}
    virtual void SaveState(nlohmann::json& jsonObject) const {}

  public:
    const UUID& GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }

    NodeValueType GetValueType() const
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

    const std::vector<Pin>& GetInputPins() const { return m_inputPins; }
    const std::vector<Pin>& GetOutputPins() const { return m_outputPins; }
    uint32_t GetInputPinsCount() const { return m_inputPins.size(); }
    uint32_t GetOutputPinsCount() const { return m_outputPins.size(); }

    const Pin& GetInputPin(size_t pinIdx) const
    {
        assert(pinIdx >= 0 && pinIdx <= m_inputPins.size());
        return m_inputPins[pinIdx];
    }

    const Pin& GetOutputPin(size_t pinIdx) const
    {
        assert(pinIdx >= 0 && pinIdx <= m_outputPins.size());
        return m_outputPins[pinIdx];
    }

    uint32_t GetInputPinIndex(const UUID& pinID) const
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

    uint32_t GetOutputPinIndex(const UUID& pinID) const
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

    virtual bool SupportsDynamicInputPins() const { return false; }
    virtual NodeValueType DynamicInputPinValueType() const { return NodeValueType::Unknown; }
    virtual std::string DynamicInputPinName() const { return ""; }

    virtual bool SupportsDynamicOutputPins() const { return false; }
    virtual NodeValueType DynamicOutputPinValueType() const { return NodeValueType::Unknown; }
    virtual std::string DynamicOutputPinName() const { return ""; }

    // ----- Renaming

    bool IsRenamable() const { return m_renamable; }

    void BeginRenaming()
    {
        assert(m_renamable);
        m_renamingInProgress = true;
        m_renamingStarted = true;
    }

    void EndRenaming()
    {
        assert(m_renamable);
        m_renamingInProgress = false;
    }

    // ----- Lifetime

    virtual void Initialize() = 0;

    /// @brief Compile the node and add it to a graph definition
    /// @param context Context for the running compilation
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const = 0;

    void SaveNodeState(nlohmann::json& json) const
    {
        if (m_renamable)
        {
            json["name"] = m_name;
        }
        SaveState(json);
    }

    void LoadNodeState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
    {
        if (m_renamable)
        {
            m_name = json["name"];
        }
        LoadState(json, pTypeRegistryService);
    }

    void DrawNode(GraphDrawingContext& ctx)
    {
        ctx.m_currentNodeWidth = CalcNodeWidth();

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
    }

    bool operator==(const EditorGraphNode& other) { return m_id == other.m_id; }
    bool operator!=(const EditorGraphNode& other) { return m_id != other.m_id; }
};
} // namespace aln