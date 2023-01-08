#pragma once

#include <common/uuid.hpp>
#include <reflection/reflected_type.hpp>
#include <reflection/type_info.hpp>

#include "pin.hpp"

namespace aln
{

// fwd
class AnimationGraphCompilationContext;
class AnimationGraphDefinition;

class EditorGraphNode : public reflect::IReflected
{
    friend class AnimationGraphEditor;

  private:
    const UUID m_id = UUID::Generate();

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

    virtual void Initialize() = 0;
    virtual void Serialize() = 0; // TODO

    /// @brief Compile the node and add it to a graph definition
    /// @param context Context for the running compilation
    virtual void Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const = 0;

    bool operator==(const EditorGraphNode& other) { return m_id == other.m_id; }
    bool operator!=(const EditorGraphNode& other) { return m_id != other.m_id; }
};
} // namespace aln