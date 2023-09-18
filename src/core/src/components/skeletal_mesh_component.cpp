#include "components/skeletal_mesh_component.hpp"

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::SkeletalMeshComponent)
ALN_REFLECT_BASE(aln::MeshComponent)
ALN_REFLECT_MEMBER(m_drawDebugSkeleton)
ALN_REFLECT_MEMBER(m_drawRootBone)
ALN_REFLECT_MEMBER(m_pMesh)
ALN_REFLECT_MEMBER(m_pSkeleton)
ALN_REGISTER_IMPL_END()

namespace aln
{
void SkeletalMeshComponent::UpdateSkinningTransforms()
{
    const auto boneCount = m_boneTransforms.size();
    for (auto i = 0; i < boneCount; ++i)
    {
        const Transform transform = m_boneTransforms[i] * m_pMesh->GetInverseBindPose()[i];
        m_skinningTransforms[i] = transform.ToMatrix();
    }
}
} // namespace aln