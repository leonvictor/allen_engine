/// Descriptors for custom color types.
#include "reflection.hpp"

#include <common/colors.hpp>

#include <glm/gtx/string_cast.hpp>
#include <string>

// Colors are registered in reflection and not common,
// only because reflection depends on common and not the other way around
// TODO: This shouldn't be the case...
// Maybe move colors to Core ?
namespace aln
{
ALN_REGISTER_PRIMITIVE(RGBAColor);
ALN_REGISTER_PRIMITIVE(RGBColor);
} // namespace aln