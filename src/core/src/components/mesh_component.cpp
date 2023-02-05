#include "components/mesh_component.hpp"

#include <graphics/device.hpp>

namespace aln
{

ALN_REGISTER_ABSTRACT_IMPL_BEGIN(MeshComponent)
ALN_REFLECT_BASE(SpatialComponent)
ALN_REGISTER_IMPL_END()

// -------------------------------------------------
// Components Methods
// -------------------------------------------------

void MeshComponent::Initialize()
{
}

void MeshComponent::Shutdown()
{
}
} // namespace aln
