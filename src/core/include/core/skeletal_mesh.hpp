#pragma once

#include "mesh.hpp"

#include <common/transform.hpp>
#include <common/types.hpp>
#include <reflection/type_info.hpp>

namespace aln
{
class SkeletalMesh : public Mesh
{
    friend class MeshLoader;
    friend class SkeletalMeshComponent;

    ALN_REGISTER_ASSET_TYPE("smsh")

  private:
      // TODO: Use StringID for boneNames
    std::vector<std::string> m_boneNames;
    std::vector<uint32_t> m_parentBoneIndices;
    
    /// @note Bind poses are in global space
    std::vector<Transform> m_bindPose;        // Mesh space -> Bone space
    std::vector<Transform> m_inverseBindPose; // Bone space -> Mesh space

  public:
    /// @brief Bind pose in global space
    const std::vector<Transform>& GetBindPose() const { return m_bindPose; }
    const std::vector<Transform>& GetInverseBindPose() const { return m_inverseBindPose; }
    size_t GetBonesCount() const { return m_bindPose.size(); }
    inline uint32_t GetParentBoneIndex(uint32_t boneIndex) const { return m_parentBoneIndices[boneIndex]; }
    
    uint32_t GetBoneIndex(const std::string& boneName) const
    {
        const auto boneCount = m_boneNames.size();
        for (auto boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            if (m_boneNames[boneIndex] == boneName)
            {
                return boneIndex;
            }
        }
        return InvalidIndex;
    }




};
} // namespace aln