#pragma once

#include <assimp/scene.h>

#include <common/transform.hpp>
#include <common/types.hpp>

#include <assert.h>
#include <string>
#include <vector>

namespace aln::assets::converter
{

class AssimpSceneContext;

/// @brief Holds raw skeleton data, used to save on disk
/// @todo Refactor to mimic the runtime skeleton and save through serialization
class AssimpSkeleton
{
    friend class AssimpSkeletonReader;

    struct BoneData
    {
        std::string m_name;
        uint32_t m_parentIndex = InvalidIndex;

        Transform m_localTransform = Transform::Identity;
        Transform m_globalTransform = Transform::Identity;
    };

  private:
    std::string m_name = ""; // A skeleton name is its root bone's one.
    std::vector<BoneData> m_boneData;
    Transform m_rootNodeGlobalTransform;

    std::string m_pathOnDisk = "";

  public:
    size_t GetBoneCount() const { return m_boneData.size(); }
    uint32_t GetBoneIndex(const std::string& boneName) const;
    const std::string& GetName() const { return m_name; }
    const std::string& GetBoneName(uint32_t boneIndex) const
    {
        assert(boneIndex < m_boneData.size());
        return m_boneData[boneIndex].m_name;
    }

    bool HasPathOnDisk() const { return !(m_pathOnDisk == ""); }

    std::string GetPathOnDisk() const
    {
        assert(HasPathOnDisk());
        return m_pathOnDisk;
    }

    bool SaveToBinary(std::string outputDirectory);

    void CalculateLocalTransforms();
    void CalculateGlobalTransforms();
    const Transform& GetRootBoneGlobalTransform() const { return m_rootNodeGlobalTransform; }
};

class AssimpSkeletonReader
{
    struct SkeletonRoot
    {
        const aiNode* m_pNode = nullptr;
        aiMatrix4x4 m_globalTransform;
    };

  private:
    static void ReadAnimationBoneHierarchy(AssimpSkeleton* pSkeleton, const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const aiNode* pNode, const aiMatrix4x4& parentNodeTransform, uint32_t parentBoneIndex);
    static SkeletonRoot GetSkeletonRootNode(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation);

  public:
    static const AssimpSkeleton* ReadSkeleton(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation);
    static const AssimpSkeleton* ReadSkeleton(const AssimpSceneContext& sceneContext, const aiMesh* pSkeletalMesh);
};
} // namespace aln::assets::converter