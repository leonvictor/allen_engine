#include "uuid.hpp"

namespace core
{
const UUID UUID::InvalidID = UUID(false);

UUID::UUID(bool isValid)
{
    m_isValid = isValid;
}

UUID::UUID() : m_ID(uuids::uuid_system_generator{}())
{
    m_isValid = true;
}
} // namespace core