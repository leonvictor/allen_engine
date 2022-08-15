#include "components/skeletal_mesh_component.hpp"

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::SkeletalMeshComponent)
ALN_REGISTER_IMPL_END()

namespace aln
{
void SkeletalMeshComponent::UpdateSkinningBuffer()
{
    // Update the transform matrices of skinning matrices
    for (auto i = 0; i < m_boneTransforms.size(); ++i)
    {
        const Transform transform = m_boneTransforms[i] * m_pMesh->GetInverseBindPose()[i];
        m_skinningTransforms[i] = transform.ToMatrix();
    }

    // Upload the new data to gpu
    // TODO: the buffer is host visible for now, which is not optimal
    // TODO: Only map once
    m_skinningBuffer.Map(0, sizeof(glm::mat4x4) * m_skinningTransforms.size());
    m_skinningBuffer.Copy(m_skinningTransforms);
    m_skinningBuffer.Unmap();
}
} // namespace aln