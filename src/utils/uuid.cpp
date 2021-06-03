#pragma once

#include <iostream>
#include <vector>

#define UUID_SYSTEM_GENERATOR
#include <stduuids.h>

namespace core
{
class UUID
{
  private:
    uuids::uuid m_ID;
    bool m_isValid;

    UUID(bool isValid)
    {
        m_isValid = isValid;
    }

  public:
    /// @brief Invalid ID
    static const UUID InvalidID;

    /// @brief Default constructor initializes a new valid UUID.
    UUID() : m_ID(uuids::uuid_system_generator{}())
    {
        m_isValid = true;
    }

    explicit UUID(std::array<uint8_t, 16> data)
    {
        m_ID = uuids::uuid(std::begin(data), std::end(data));
        m_isValid = true;
    }

    explicit UUID(uint8_t data[16])
    {
        uint8_t cache[16];
        memcpy(cache, data, 16 * sizeof(uint8_t));

        m_ID = uuids::uuid(std::begin(cache), std::end(cache));
        m_isValid = true;
    }

    bool IsValid() const { return m_isValid; }

    bool operator==(const UUID& other) const
    {
        if (!other.IsValid())
        {
            // An invalid UUID is only equal to another invalid one
            return !IsValid();
        }
        else
        {
            // Both UUIDs are valid and there inner id are equal
            return IsValid() && m_ID == other.m_ID;
        }
    }

    bool operator!=(const UUID& other) const { return !operator==(other); }

    friend std::ostream& operator<<(std::ostream& os, const UUID& uuid);
};

std::ostream& operator<<(std::ostream& os, const UUID& uuid)
{
    if (uuid.m_isValid)
    {
        os << "Invalid UUID";
    }
    else
    {
        os << "Valid UUID (" << uuid.m_ID << ")";
    }
    return os;
}
const UUID UUID::InvalidID = UUID(false);
} // namespace core