#pragma once

#include <common/uuid.hpp>
#include <string>

namespace IMNODES_NAMESPACE
{
using ID = aln::UUID;
static const ID INVALID_ID;

inline void PushID(ID id) { ImGui::PushID(id.ToString().c_str()); }

inline std::string IDToString(ID id) { return id.ToString(); }

inline ID IDFromString(const std::string& str) { return ID(str); }

} // namespace IMNODES_NAMESPACE