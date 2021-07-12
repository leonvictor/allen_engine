#pragma once

#include <iostream>
#include <vector>

#include <gsl/span>
#include <stduuids.h>

namespace core
{
class UUID
{
  private:
    uuids::uuid m_ID;

    UUID(bool isValid);

  public:
    static const UUID InvalidID;

    UUID();

    explicit UUID(std::array<uint8_t, 16> data)
    {
        m_ID = uuids::uuid(std::begin(data), std::end(data));
    }

    explicit UUID(uint8_t data[16])
    {
        uint8_t cache[16];
        memcpy(cache, data, 16 * sizeof(uint8_t));

        m_ID = uuids::uuid(std::begin(cache), std::end(cache));
    }

    inline bool IsValid() const { return !m_ID.is_nil(); }

    inline bool operator==(const UUID& other) const { return m_ID == other.m_ID; }
    inline bool operator!=(const UUID& other) const { return !operator==(other); }

    friend bool operator<(const UUID& l, const UUID& r) { return l.m_ID < r.m_ID; }

    friend std::ostream& operator<<(std::ostream& os, const UUID& uuid)
    {
        if (!uuid.IsValid())
        {
            os << "Invalid UUID";
        }
        else
        {
            os << "UUID (" << uuid.m_ID << ")";
        }
        return os;
    }
};
} // namespace core