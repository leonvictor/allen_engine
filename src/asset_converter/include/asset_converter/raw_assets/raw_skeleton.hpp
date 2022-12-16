#pragma once

#include <assimp/scene.h>

#include <common/serialization/binary_archive.hpp>
#include <common/transform.hpp>
#include <common/types.hpp>

#include <assert.h>
#include <string>
#include <vector>

#include "raw_asset.hpp"

namespace aln::assets::converter
{

class AssimpSceneContext;

/// @brief Intermediate Skeleton data representation, built by the converter, then serialized.
// It can be deserialized directly into the runtime format
class RawSkeleton : public IRawAsset
{
    friend class AssimpSkeletonReader;

  private:
    std::string m_name;

    // Conversion data
    /// @todo: Is that useful ?
    Transform m_rootNodeGlobalTransform;

    // Runtime data
    std::vector<std::string> m_boneNames;
    std::vector<uint32_t> m_parentBoneIndices;

    std::vector<Transform> m_localReferencePose;
    std::vector<Transform> m_globalReferencePose;

  public:
    size_t GetBoneCount() const { return m_boneNames.size(); }
    uint32_t GetBoneIndex(const std::string& boneName) const;
    const std::string& GetBoneName(uint32_t boneIndex) const
    {
        assert(boneIndex < m_boneNames.size());
        return m_boneNames[boneIndex];
    }

    void CalculateLocalTransforms();
    void CalculateGlobalTransforms();

    const Transform& GetRootBoneGlobalTransform() const { return m_rootNodeGlobalTransform; }
    void Serialize(BinaryMemoryArchive& archive) final override;
};

class AssimpSkeletonReader
{
    struct SkeletonRoot
    {
        const aiNode* m_pNode = nullptr;
        aiMatrix4x4 m_globalTransform;
    };

  private:
    static void ReadAnimationBoneHierarchy(RawSkeleton* pSkeleton, const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const aiNode* pNode, const aiMatrix4x4& parentNodeTransform, uint32_t parentBoneIndex);
    static SkeletonRoot GetSkeletonRootNode(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation);

  public:
    static const RawSkeleton* ReadSkeleton(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation);
    static const RawSkeleton* ReadSkeleton(const AssimpSceneContext& sceneContext, const aiMesh* pSkeletalMesh);
};
} // namespace aln::assets::converter