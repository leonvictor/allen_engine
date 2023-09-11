#include "components/static_mesh_component.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(COMPONENTS, StaticMeshComponent)
ALN_REFLECT_BASE(MeshComponent)
ALN_REFLECT_MEMBER(m_pMesh)
ALN_REGISTER_IMPL_END()
} // namespace aln