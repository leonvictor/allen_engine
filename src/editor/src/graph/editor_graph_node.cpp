#include "graph/editor_graph_node.hpp"

#include "graph/editor_graph.hpp"

namespace aln
{
ALN_REGISTER_ABSTRACT_IMPL_BEGIN(EditorGraphNode);
ALN_REGISTER_IMPL_END();

EditorGraphNode::~EditorGraphNode()
{
    assert(m_pChildGraph == nullptr);
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

} // namespace aln