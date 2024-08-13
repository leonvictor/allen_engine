#pragma once

#include "graph_drawing_context.hpp"
#include "pin.hpp"

#include <common/serialization/json.hpp>
#include <common/uuid.hpp>
#include <reflection/reflected_type.hpp>
#include <reflection/type_info.hpp>

namespace aln
{

class EditorGraph;

/// @brief Base class for editor graphs' nodes
class EditorGraphNode : public reflect::IReflected
{
    ALN_REGISTER_TYPE();

    friend class EditorGraph;

  private:
    UUID m_id = UUID::Generate();
    Vector<Pin> m_inputPins;
    Vector<Pin> m_outputPins;

    EditorGraph* m_pOwningGraph = nullptr; // The graph this node is in
    EditorGraph* m_pChildGraph = nullptr;

    // TODO: This is an awful lot of state
    bool m_renamingStarted = false;
    bool m_renamingInProgress = false;

    float CalcNodeWidth() const;

    virtual void PushNodeStyle(const GraphDrawingContext& ctx) const;
    virtual void PopNodeStyle(const GraphDrawingContext& ctx) const;

  protected:
    std::string m_name;

    // ----
    const Pin& AddInputPin(NodeValueType valueType, std::string name = "");
    const Pin& AddOutputPin(NodeValueType valueType, std::string name = "", bool allowMultipleLinks = false);

    const Pin& AddDynamicInputPin(NodeValueType valueType, std::string name = "");
    void RemoveDynamicInputPin(const UUID& pinID);

    // TODO: Dynamic output

    // ---- Dynamic pins creation/deletion callbacks
    virtual void OnDynamicInputPinCreated(const UUID& pinID) {}
    virtual void OnDynamicInputPinRemoved(const UUID& pinID) {}
    virtual void OnDynamicOutputPinCreated(const UUID& pinID) {}
    virtual void OnDynamicOuputPinRemoved(const UUID& pinID) {}

    // ---- Custom drawing
    virtual void DrawNodeTitleBar(const GraphDrawingContext& ctx);
    virtual bool DrawContent(const GraphDrawingContext& ctx);
    virtual bool DrawPin(const Pin& pin, const GraphDrawingContext& ctx);

    // ---- Custom serialization
    virtual void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) {}
    virtual void SaveState(JSON& jsonObject) const {}

  public:
    // TODO: In initialize/shutdown?
    ~EditorGraphNode();

    const UUID& GetID() const { return m_id; }
    virtual const std::string& GetName() const { return m_name; }
    NodeValueType GetValueType() const;

    // ---- Graph hierarchy
    const EditorGraph* GetOwningGraph() const { return m_pOwningGraph; }
    bool HasChildGraph() const { return m_pChildGraph != nullptr; }
    EditorGraph* GetChildGraph() const { return m_pChildGraph; }
    /// @brief Set a graph as child, taking ownership of it
    void SetChildGraph(EditorGraph* pChildGraph);

    // ---- Graph handling
    const Vector<Pin>& GetInputPins() const { return m_inputPins; }
    const Vector<Pin>& GetOutputPins() const { return m_outputPins; }
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

    uint32_t GetInputPinIndex(const UUID& pinID) const;
    uint32_t GetOutputPinIndex(const UUID& pinID) const;

    virtual bool SupportsDynamicInputPins() const { return false; }
    virtual NodeValueType DynamicInputPinValueType() const { return NodeValueType::Unknown; }
    virtual std::string DynamicInputPinName() const { return ""; }

    virtual bool SupportsDynamicOutputPins() const { return false; }
    virtual NodeValueType DynamicOutputPinValueType() const { return NodeValueType::Unknown; }
    virtual std::string DynamicOutputPinName() const { return ""; }

    // ----- Renaming
    virtual bool IsRenamable() const { return false; }

    void BeginRenaming()
    {
        assert(IsRenamable());
        m_renamingInProgress = true;
        m_renamingStarted = true;
    }

    void EndRenaming()
    {
        assert(IsRenamable());
        m_renamingInProgress = false;
    }

    // ----- Lifetime
    /// @note Nodes can either be initialized through this function or by deserialization
    virtual void Initialize() = 0;
    virtual void Shutdown();

    // ---- State serialization
    void SaveNodeState(JSON& json) const;
    void LoadNodeState(const JSON& json, const TypeRegistryService* pTypeRegistryService);

    // ---- Drawing
    void DrawNode(GraphDrawingContext& ctx);

    bool operator==(const EditorGraphNode& other) { return m_id == other.m_id; }
    bool operator!=(const EditorGraphNode& other) { return m_id != other.m_id; }
};
} // namespace aln