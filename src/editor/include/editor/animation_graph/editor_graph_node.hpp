#pragma once

#include <common/uuid.hpp>

#include "pin.hpp"

namespace aln
{
class EditorGraphNode
{
    friend class AnimationGraphEditor;

  private:
    const UUID m_id = UUID::Generate();
    EditorGraphNode* m_pParentNode = nullptr;

    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;

  protected:
    std::string m_name;

    void AddInputPin(PinValueType valueType, std::string name = "")
    {
        auto& pin = m_inputPins.emplace_back();
        pin.m_type = Pin::Type::In;
        pin.m_valueType = valueType;
    }

    void AddOutputPin(PinValueType valueType, std::string name = "")
    {
        auto& pin = m_outputPins.emplace_back();
        pin.m_type = Pin::Type::Out;
        pin.m_valueType = valueType;
    }

  public:
    const UUID& GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }

    virtual void Initialize() = 0;
    virtual void Serialize() = 0; // TODO

    bool IsRoot() const { return m_pParentNode == nullptr; }

    bool operator==(const EditorGraphNode& other) { return m_id == other.m_id; }
    bool operator!=(const EditorGraphNode& other) { return m_id != other.m_id; }
};
} // namespace aln