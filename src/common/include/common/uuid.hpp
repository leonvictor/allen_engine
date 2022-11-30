#pragma once

#include <assert.h>
#include <iostream>
#include <span>
#include <vector>

#include <stduuids.h>

namespace aln
{
class UUID
{
  private:
    uuids::uuid m_ID;

  public:
    /// @brief Default construct an invalid id
    UUID() { assert(!IsValid()); }
    UUID(std::array<uint8_t, 16> data) : m_ID(data) {}
    UUID(std::span<uint8_t, 16> data) : m_ID(data) {}

    UUID(const std::string& str) : m_ID(uuids::uuid::from_string(str).value()) {}

    static UUID Generate()
    {
        UUID id;
        id.m_ID = uuids::uuid_system_generator{}();
        return id;
    }

    static UUID InvalidID() { return UUID(); }

    inline std::string ToString() const { return uuids::to_string(m_ID); }
    inline bool IsValid() const { return !m_ID.is_nil(); }

    inline bool operator==(const UUID& other) const { return m_ID == other.m_ID; }
    inline bool operator!=(const UUID& other) const { return !operator==(other); }
    friend bool operator<(const UUID& l, const UUID& r) { return l.m_ID < r.m_ID; }
    friend bool operator>(const UUID& l, const UUID& r) { return r.m_ID < l.m_ID; }

    friend std::ostream& operator<<(std::ostream& os, const UUID& uuid)
    {
        if (!uuid.IsValid())
        {
            os << "Invalid UUID";
        }
        else
        {
            os << uuid.m_ID;
        }
        return os;
    }
};

} // namespace aln