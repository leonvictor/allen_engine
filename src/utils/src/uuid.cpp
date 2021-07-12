#include "uuid.hpp"
#include <assert.h>

namespace core
{

UUID::UUID(bool isValid)
{
    assert(!isValid); // The private constructor is only used to create the static invalid ID member.
}

UUID::UUID() : m_ID(uuids::uuid_system_generator{}()) {}
} // namespace core