#include "components/skeletal_mesh_component.hpp"

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::SkeletalMeshComponent)
ALN_REFLECT_BASE(aln::MeshComponent)
ALN_REFLECT_MEMBER(m_drawDebugSkeleton, Draw Debug Skeleton)
ALN_REFLECT_MEMBER(m_drawRootBone, Draw Root Bone)
ALN_REFLECT_MEMBER(m_pMesh, Mesh)
ALN_REFLECT_MEMBER(m_pSkeleton, Skeleton)
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