#include "assimp_skeleton.hpp"

#include "assimp_scene_context.hpp"

#include <assets/asset_system/skeleton_asset.hpp>

namespace aln::assets::converter
{

uint32_t AssimpSkeleton::GetBoneIndex(const std::string& boneName) const
{
    for (size_t boneIndex = 0; boneIndex < m_boneData.size(); ++boneIndex)
    {
        if (boneName == m_boneData[boneIndex].m_name)
        {
            return boneIndex;
        }
    }
    return InvalidIndex;
}

bool AssimpSkeleton::SaveToBinary(std::string outputDirectory)
{
    assert(!HasPathOnDisk());

    std::vector<Transform> referencePose;
    referencePose.reserve(GetBoneCount());

    SkeletonInfo skeletonInfo;
    skeletonInfo.assetPath = outputDirectory + "/" + m_name + ".skel";

    for (auto& bone : m_boneData)
    {
        skeletonInfo.boneNames.push_back(bone.m_name);
        skeletonInfo.boneParentIndices.push_back(bone.m_parentIndex);
        referencePose.push_back(bone.m_localTransform);
    }

    auto file = SkeletonConverter::Pack(&skeletonInfo, referencePose);
    if (SaveBinaryFile(skeletonInfo.assetPath, file))
    {
        m_pathOnDisk = skeletonInfo.assetPath;
        return true;
    }

    return false;
}

void AssimpSkeleton::CalculateLocalTransforms()
{
    m_boneData[0].m_localTransform = m_boneData[0].m_globalTransform;
    for (auto boneIdx = GetBoneCount() - 1; boneIdx > 0; --boneIdx)
    {
        auto& bone = m_boneData[boneIdx];
        auto& parentBone = m_boneData[bone.m_parentIndex];

        bone.m_localTransform = parentBone.m_globalTransform.GetInverse() * bone.m_globalTransform;
    }
}

void AssimpSkeleton::CalculateGlobalTransforms()
{
    m_boneData[0].m_globalTransform = m_boneData[0].m_localTransform;
    for (auto boneIdx = 0; boneIdx < GetBoneCount(); ++boneIdx)
    {
        auto& bone = m_boneData[boneIdx];
        auto& parentBone = m_boneData[bone.m_parentIndex];

        bone.m_globalTransform = parentBone.m_globalTransform * bone.m_localTransform;
    }
}

/// ---------------
/// Skeleton reader
/// ---------------

const AssimpSkeleton* AssimpSkeletonReader::ReadSkeleton(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation)
{
    auto skeletonRoot = GetSkeletonRootNode(sceneContext, pAnimation);
    assert(skeletonRoot.m_pNode != nullptr);

    auto skeletonName = std::string(skeletonRoot.m_pNode->mName.C_Str());

    // Look for an existing skeleton with this root
    AssimpSkeleton* pSkeleton = nullptr;
    if (!sceneContext.TryGetSkeleton(skeletonName, pSkeleton))
    {
        // Add the root
        pSkeleton->m_name = skeletonName;
        pSkeleton->m_rootNodeGlobalTransform = sceneContext.DecomposeMatrix(skeletonRoot.m_globalTransform);

        auto& rootData = pSkeleton->m_boneData.emplace_back();
        rootData.m_name = skeletonName;
        rootData.m_globalTransform = Transform::Identity;
        rootData.m_localTransform = Transform::Identity;
        rootData.m_parentIndex = InvalidIndex;

        // Traverse the hierarchy to build the skeleton
        ReadAnimationBoneHierarchy(pSkeleton, sceneContext, pAnimation, skeletonRoot.m_pNode, skeletonRoot.m_globalTransform, 0);

        // Save the skeleton
        if (!pSkeleton->SaveToBinary(sceneContext.GetOutputDirectory().string()))
        {
            throw std::runtime_error("Failed to save skeleton asset.");
        }
    }
    return pSkeleton;
}

AssimpSkeletonReader::SkeletonRoot AssimpSkeletonReader::GetSkeletonRootNode(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation)
{
    struct SkeletonRootNodeFinder
    {
        const AssimpSceneContext* m_pSceneContext;
        const aiAnimation* m_pAnimation;

        SkeletonRoot m_skeletonRoot;

        void FindSkeletonRootNode(const aiNode* pNode, const aiMatrix4x4& parentGlobalTransform)
        {
            if (m_skeletonRoot.m_pNode != nullptr)
            {
                return; // Root already found
            }

            auto& nodeLocalTransform = pNode->mTransformation;
            auto nodeGlobalTranform = nodeLocalTransform * parentGlobalTransform;

            for (auto channelIndex = 0; channelIndex < m_pAnimation->mNumChannels; ++channelIndex)
            {
                auto pChannel = m_pAnimation->mChannels[channelIndex];
                if (pNode->mName == pChannel->mNodeName)
                {
                    m_skeletonRoot.m_pNode = pNode->mParent;
                    m_skeletonRoot.m_globalTransform = parentGlobalTransform;
                    return;
                }
            }

            for (auto childIndex = 0; childIndex < pNode->mNumChildren; ++childIndex)
            {
                FindSkeletonRootNode(pNode->mChildren[childIndex], nodeGlobalTranform);
            }
        }

        SkeletonRootNodeFinder(const AssimpSceneContext* pSceneContext, const aiAnimation* pAnimation) : m_pSceneContext(pSceneContext), m_pAnimation(pAnimation)
        {
            const auto& pRootNode = m_pSceneContext->GetRootNode();
            FindSkeletonRootNode(pRootNode, pRootNode->mTransformation);
            // TODO: Handle cases where there was no common parent node
        }
    };

    auto rootFinder = SkeletonRootNodeFinder(&sceneContext, pAnimation);
    return rootFinder.m_skeletonRoot;
}

void AssimpSkeletonReader::ReadAnimationBoneHierarchy(AssimpSkeleton* pSkeleton, const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const aiNode* pNode, const aiMatrix4x4& parentNodeTransform, uint32_t parentBoneIndex)
{
    auto nodeLocalTransform = pNode->mTransformation;
    auto globalNodeTransform = nodeLocalTransform * parentNodeTransform; // inv

    bool boneFound = false;
    for (auto channelIndex = 0; channelIndex < pAnimation->mNumChannels && !boneFound; ++channelIndex)
    {
        auto pChannel = pAnimation->mChannels[channelIndex];
        if (pNode->mName == pChannel->mNodeName)
        {
            auto& data = pSkeleton->m_boneData.emplace_back();

            data.m_name = std::string(pNode->mName.C_Str());
            data.m_parentIndex = parentBoneIndex;
            data.m_localTransform = sceneContext.DecomposeMatrix(nodeLocalTransform);
            data.m_globalTransform = sceneContext.DecomposeMatrix(globalNodeTransform);

            parentBoneIndex = pSkeleton->m_boneData.size() - 1;
            boneFound = true;
        }
    }

    for (auto childIndex = 0; childIndex < pNode->mNumChildren; ++childIndex)
    {
        ReadAnimationBoneHierarchy(pSkeleton, sceneContext, pAnimation, pNode->mChildren[childIndex], globalNodeTransform, parentBoneIndex);
    }
}

const AssimpSkeleton* AssimpSkeletonReader::ReadSkeleton(const AssimpSceneContext& sceneContext, const aiMesh* pSkeletalMesh)
{
    assert(pSkeletalMesh->HasBones());

    // Initialize the root node
    auto pRootBone = pSkeletalMesh->mBones[0]->mArmature;
    auto skeletonName = std::string(pRootBone->mName.C_Str());

    AssimpSkeleton* pSkeleton = nullptr;
    if (!sceneContext.TryGetSkeleton(skeletonName, pSkeleton))
    {
        pSkeleton->m_name = skeletonName;
        pSkeleton->m_rootNodeGlobalTransform = sceneContext.GetGlobalTransform(pRootBone);

        pSkeleton->m_boneData.reserve(pSkeletalMesh->mNumBones + 1);

        auto& rootData = pSkeleton->m_boneData.emplace_back();
        rootData.m_localTransform = Transform::Identity;
        rootData.m_globalTransform = Transform::Identity;
        rootData.m_name = skeletonName;
        rootData.m_parentIndex = InvalidIndex;

        // Initialize the rest of the bones
        for (size_t meshBoneIndex = 0; meshBoneIndex < pSkeletalMesh->mNumBones; ++meshBoneIndex)
        {
            auto pBone = pSkeletalMesh->mBones[meshBoneIndex];
            auto boneName = std::string(pBone->mName.C_Str());

            auto parentBoneName = std::string(pBone->mNode->mParent->mName.C_Str());
            auto parentBoneIndex = pSkeleton->GetBoneIndex(parentBoneName);

            auto inverseBindPose = sceneContext.DecomposeMatrix(pBone->mOffsetMatrix);
            auto bindPose = Transform(glm::inverse(sceneContext.ToGLM(pBone->mOffsetMatrix)));

            auto& data = pSkeleton->m_boneData.emplace_back();
            data.m_name = boneName;
            data.m_parentIndex = parentBoneIndex;
            data.m_globalTransform = bindPose;
        }

        pSkeleton->CalculateLocalTransforms();

        // Save skeleton
        // TODO: Use skeleton static asset type for extension
        if (!pSkeleton->SaveToBinary(sceneContext.GetOutputDirectory().string()))
        {
            throw std::runtime_error("Failed to save skeleton asset.");
        }
    }

    assert(pSkeleton != nullptr);
    return pSkeleton;
}
} // namespace aln::assets::converter