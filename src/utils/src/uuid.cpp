#include "uuid.hpp"
#include <assert.h>

namespace aln::utils
{

UUID::UUID(bool isValid)
{
    assert(!isValid); // The private constructor is only used to create the static invalid ID member.
}

UUID::UUID() : m_ID(uuids::uuid_system_generator{}()) {}

bool UUID::IsValid() const { return !m_ID.is_nil(); }

std::string UUID::ToString() const { return uuids::to_string(m_ID); }

} // namespace aln::utils