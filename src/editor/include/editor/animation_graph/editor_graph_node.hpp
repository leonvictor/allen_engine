#pragma once

#include <common/types.hpp>
#include <common/uuid.hpp>

#include <reflection/reflected_type.hpp>
#include <reflection/type_info.hpp>

#include "pin.hpp"

#include <nlohmann/json.hpp>

#include <animation_graph/animation_graph_compilation_context.hpp>

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

  protected:
    std::string m_name;

    void AddInputPin(PinValueType valueType, std::string name = "", bool allowMultipleLinks = false)
    {
        auto& pin = m_inputPins.emplace_back();
        pin.m_type = Pin::Type::In;
        pin.m_valueType = valueType;
        pin.m_name = name;
        pin.m_allowMultipleLinks = allowMultipleLinks;
    }

    void AddOutputPin(PinValueType valueType, std::string name = "", bool allowMultipleLinks = false)
    {
        auto& pin = m_outputPins.emplace_back();
        pin.m_type = Pin::Type::Out;
        pin.m_valueType = valueType;
        pin.m_name = name;
        pin.m_allowMultipleLinks = allowMultipleLinks;
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

    uint32_t GetInputPinIndex(const UUID& pinID)
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

    uint32_t GetOutputPinIndex(const UUID& pinID)
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

    virtual void Initialize() = 0;

    /// @brief Compile the node and add it to a graph definition
    /// @param context Context for the running compilation
    virtual NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition* pGraphDefinition) const = 0;

    void SaveNodeState(nlohmann::json& jsonObject)
    {
        // Custom ...
        SaveState(jsonObject);
    }

    void LoadNodeState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService)
    {
        LoadState(json, pTypeRegistryService);
    }

    // Customizable parts
    virtual void LoadState(const nlohmann::json& json, const TypeRegistryService* pTypeRegistryService) {}
    virtual void SaveState(nlohmann::json& jsonObject) {}

    bool operator==(const EditorGraphNode& other) { return m_id == other.m_id; }
    bool operator!=(const EditorGraphNode& other) { return m_id != other.m_id; }
};
} // namespace aln