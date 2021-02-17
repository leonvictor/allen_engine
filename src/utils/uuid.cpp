#pragma once

#define UUID_SYSTEM_GENERATOR
#include <uuids.h>

class UUID
{
  private:
    uuids::uuid m_id;
    bool m_isValid;

    UUID(bool isValid)
    {
        m_isValid = false;
    }

  public:
    static const UUID InvalidID;

    UUID() : m_id(uuids::uuid_system_generator{}())
    {
        m_isValid = true;
    }

    // TODO: copy constructor is explicit ?
    // UUID(const UUID& uuid)
    // {
    //     m_id = uuid.m_id;
    //     m_isValid = uuid.m_isValid;
    // }

    bool IsValid() const
    {
        return m_isValid;
    }

    bool operator==(const UUID& other) const
    {
        return m_id == other.m_id;
    }

    bool operator!=(const UUID& other) const
    {
        return !operator==(other);
    }
};

const UUID UUID::InvalidID = UUID(false);