#pragma once

#include <common/uuid.hpp>

#include "pin_value_types.hpp"

namespace aln
{

class Pin
{
  private:
    friend class EditorGraphNode;

    enum class Type : uint8_t
    {
        In,
        Out
    };

  public:
    inline bool IsInput() const { return m_type == Type::In; }
    inline bool IsOutput() const { return m_type == Type::Out; }
    inline UUID GetID() const { return m_id; }
    inline const std::string& GetName() const { return m_name; }
    inline const PinValueType GetValueType() const { return m_valueType; }
    inline bool AllowsMultipleLinks() const { return m_allowMultipleLinks; }

  private:
    const UUID m_id = UUID::Generate();
    std::string m_name;
    Type m_type = Type::In;
    PinValueType m_valueType = PinValueType::None;
    bool m_allowMultipleLinks = false;
};
} // namespace aln