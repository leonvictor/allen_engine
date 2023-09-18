#pragma once

#include "containers/span.hpp"

#include <aln_common_export.h>

#include <stduuids.h>

#include <assert.h>
#include <iostream>

namespace aln
{
class ALN_COMMON_EXPORT UUID
{
    friend struct eastl::hash<UUID>;

  private:
    uuids::uuid m_ID;

  public:
    /// @brief Default construct an invalid id
    UUID() { assert(!IsValid()); }
    UUID(const Span<uint8_t, 16>& data) : m_ID(data.begin(), data.end()) {}
    UUID(const std::string& str) : m_ID(uuids::uuid::from_string(str).value()) {}
    
    static UUID Generate()
    {
        UUID id;
        id.m_ID = uuids::uuid_system_generator{}();
        return id;
    }

    static const UUID InvalidID;

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

namespace eastl
{
template <>
struct hash<aln::UUID>
{
    size_t operator()(const aln::UUID& id) const { return std::hash<uuids::uuid>{}(id.m_ID); }
};
} // namespace eastl