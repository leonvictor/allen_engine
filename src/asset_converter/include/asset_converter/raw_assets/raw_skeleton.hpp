#pragma once

#include "raw_asset.hpp"

#include <common/containers/vector.hpp>
#include <common/serialization/binary_archive.hpp>
#include <common/transform.hpp>
#include <common/types.hpp>

#include <assimp/scene.h>

#include <assert.h>
#include <string>

namespace aln::assets::converter
{

class AssimpSceneContext;
class RawSkeletalMesh;

/// @brief Intermediate Skeleton data representation, built by the converter, then serialized.
// It can be deserialized directly into the runtime format
class RawSkeleton : public IRawAsset
{
    friend class AssimpSkeletonReader;
    friend class RawSkeletalMesh;

  private:
    std::string m_name;

    // Conversion data
    /// @todo: Is that useful ?
    Transform m_rootNodeGlobalTransform;

    // Runtime data
    Vector<std::string> m_boneNames;
    Vector<uint32_t> m_parentBoneIndices;

    Vector<Transform> m_localReferencePose;
    Vector<Transform> m_globalReferencePose;

  public:
    size_t GetBonesCount() const { return m_boneNames.size(); }
    uint32_t GetBoneIndex(const std::string& boneName) const;
    const std::string& GetBoneName(uint32_t boneIndex) const
    {
        assert(boneIndex < m_boneNames.size());
        return m_boneNames[boneIndex];
    }

    void CalculateLocalTransforms();
    void CalculateGlobalTransforms();

    /// @brief Reorder bones so that parents appear before their children
    void SortBones();

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
    static void ReadSkeleton(const AssimpSceneContext& sceneContext, const aiMesh* pSkeletalMesh, RawSkeleton* pOutSkeleton);
};
} // namespace aln::assets::converter