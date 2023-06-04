#include "raw_assets/raw_skeleton.hpp"

#include "assimp_scene_context.hpp"

#include <assets/asset_archive_header.hpp>
#include <common/serialization/binary_archive.hpp>
#include <common/serialization/compression.hpp>

namespace aln::assets::converter
{

uint32_t RawSkeleton::GetBoneIndex(const std::string& boneName) const
{
    auto boneCount = m_boneNames.size();
    for (auto boneIndex = 0; boneIndex < boneCount; ++boneIndex)
    {
        if (boneName == m_boneNames[boneIndex])
        {
            return boneIndex;
        }
    }
    return InvalidIndex;
}

void RawSkeleton::Serialize(BinaryMemoryArchive& archive)
{
    archive << m_boneNames;
    archive << m_parentBoneIndices;
    archive << m_globalReferencePose;
    archive << m_localReferencePose;
}

void RawSkeleton::CalculateLocalTransforms()
{
    m_localReferencePose.resize(m_boneNames.size());
    m_localReferencePose[0] = m_globalReferencePose[0];
    for (auto boneIdx = GetBoneCount() - 1; boneIdx > 1; --boneIdx)
    {
        auto& parentBoneIdx = m_parentBoneIndices[boneIdx];
        m_localReferencePose[boneIdx] = m_globalReferencePose[parentBoneIdx].GetInverse() * m_globalReferencePose[boneIdx];
    }
}

void RawSkeleton::CalculateGlobalTransforms()
{
    m_globalReferencePose.resize(m_boneNames.size());
    m_globalReferencePose[0] = m_localReferencePose[0];
    for (auto boneIdx = 1; boneIdx < GetBoneCount(); ++boneIdx)
    {
        auto& parentBoneIdx = m_parentBoneIndices[boneIdx];
        m_globalReferencePose[boneIdx] = m_globalReferencePose[parentBoneIdx] * m_localReferencePose[boneIdx];
    }
}

void RawSkeleton::SortBones()
{
    assert(!m_parentBoneIndices.empty());

    // Sort bones indices so that parents appear before children
    // Inverse mappings are also generated to actually update parent indices later on

    auto boneCount = GetBoneCount();
    
    std::vector<size_t> sortedToOriginal;
    sortedToOriginal.reserve(boneCount);
    std::vector<size_t> originalToSorted;
    originalToSorted.resize(boneCount);

    // Initialize with root nodes
    for (auto boneIdx = 0; boneIdx < boneCount; ++boneIdx)
    {
        if (m_parentBoneIndices[boneIdx] == InvalidIndex)
        {
            originalToSorted[boneIdx] = sortedToOriginal.size(); 
            sortedToOriginal.push_back(boneIdx);
        }
    }
    
    size_t currentParentsStart = 0;
    while (sortedToOriginal.size() < boneCount)
    {
        size_t currentParentsEnd = sortedToOriginal.size();
        
        // Append all direct children of the last group of added bones
        for (auto parentBoneIdx = currentParentsStart; parentBoneIdx < currentParentsEnd; ++parentBoneIdx)
        {
            for (auto childBoneIdx = 0; childBoneIdx < boneCount; ++childBoneIdx)
            {
                if (m_parentBoneIndices[childBoneIdx] == sortedToOriginal[parentBoneIdx])
                {
                    originalToSorted[childBoneIdx] = sortedToOriginal.size(); 
                    sortedToOriginal.push_back(childBoneIdx);
                }
            }
        }
        currentParentsStart = currentParentsEnd;
    }

    // Update bone data to sorted order
    auto boneNames = m_boneNames;
    auto parentBoneIndices = m_parentBoneIndices;
    auto globalReferencePose = m_globalReferencePose;
    auto localReferencePose = m_localReferencePose;

    for (auto newBoneIndex = 0; newBoneIndex < sortedToOriginal.size(); ++newBoneIndex)
    {
        auto oldBoneIndex = sortedToOriginal[newBoneIndex];
        auto oldParentBoneIndex = parentBoneIndices[oldBoneIndex];
        
        if (oldParentBoneIndex == InvalidIndex)
        {
            m_parentBoneIndices[newBoneIndex] = InvalidIndex;
        }
        else
        {
            m_parentBoneIndices[newBoneIndex] = originalToSorted[oldParentBoneIndex];
        }
        
        m_boneNames[newBoneIndex] = boneNames[oldBoneIndex];
        m_globalReferencePose[newBoneIndex] = globalReferencePose[oldBoneIndex];
        m_localReferencePose[newBoneIndex] = localReferencePose[oldBoneIndex];
    }
}


/// ---------------
/// Skeleton reader
/// ---------------

const RawSkeleton* AssimpSkeletonReader::ReadSkeleton(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation)
{
    auto skeletonRoot = GetSkeletonRootNode(sceneContext, pAnimation);
    assert(skeletonRoot.m_pNode != nullptr);

    auto skeletonName = std::string(skeletonRoot.m_pNode->mName.C_Str());

    auto exportPath = sceneContext.GetOutputDirectory() / skeletonName;
    exportPath.replace_extension("skel");

    // Look for an existing skeleton with this root
    RawSkeleton* pSkeleton = nullptr;
    if (!sceneContext.TryGetSkeleton(skeletonName, pSkeleton))
    {
        pSkeleton->m_name = skeletonName;
        pSkeleton->m_id = AssetID(exportPath.string());
        pSkeleton->m_rootNodeGlobalTransform = sceneContext.DecomposeMatrix(skeletonRoot.m_globalTransform);

        // Add root data
        pSkeleton->m_boneNames.push_back(skeletonName);
        pSkeleton->m_parentBoneIndices.push_back(InvalidIndex);
        pSkeleton->m_globalReferencePose.push_back(Transform::Identity);
        pSkeleton->m_localReferencePose.push_back(Transform::Identity);

        // Traverse the hierarchy to build the skeleton
        ReadAnimationBoneHierarchy(pSkeleton, sceneContext, pAnimation, skeletonRoot.m_pNode, skeletonRoot.m_globalTransform, 0);

        // Save the skeleton
        // TODO: Check against existing skeletons
        std::vector<std::byte> data;
        auto dataStream = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        pSkeleton->Serialize(dataStream);

        // TODO: Use skeleton static asset type
        auto header = AssetArchiveHeader("skel");
        auto archive = BinaryFileArchive(pSkeleton->m_id.GetAssetPath(), IBinaryArchive::IOMode::Write);
        archive << header << data;
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

void AssimpSkeletonReader::ReadAnimationBoneHierarchy(RawSkeleton* pSkeleton, const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const aiNode* pNode, const aiMatrix4x4& parentNodeTransform, uint32_t parentBoneIndex)
{
    auto nodeLocalTransform = pNode->mTransformation;
    auto globalNodeTransform = nodeLocalTransform * parentNodeTransform; // inv

    bool boneFound = false;
    for (auto channelIndex = 0; channelIndex < pAnimation->mNumChannels && !boneFound; ++channelIndex)
    {
        auto pChannel = pAnimation->mChannels[channelIndex];
        if (pNode->mName == pChannel->mNodeName)
        {
            pSkeleton->m_boneNames.push_back(std::string(pNode->mName.C_Str()));
            pSkeleton->m_parentBoneIndices.push_back(parentBoneIndex);
            pSkeleton->m_globalReferencePose.push_back(sceneContext.DecomposeMatrix(globalNodeTransform));
            pSkeleton->m_localReferencePose.push_back(sceneContext.DecomposeMatrix(nodeLocalTransform));

            parentBoneIndex = pSkeleton->m_boneNames.size() - 1;
            boneFound = true;
        }
    }

    for (auto childIndex = 0; childIndex < pNode->mNumChildren; ++childIndex)
    {
        ReadAnimationBoneHierarchy(pSkeleton, sceneContext, pAnimation, pNode->mChildren[childIndex], globalNodeTransform, parentBoneIndex);
    }
}

const RawSkeleton* AssimpSkeletonReader::ReadSkeleton(const AssimpSceneContext& sceneContext, const aiMesh* pSkeletalMesh)
{
    assert(pSkeletalMesh->HasBones());
    assert(pSkeletalMesh != nullptr);

    // Initialize the root node
    auto pRootBone = pSkeletalMesh->mBones[0]->mArmature;
    auto skeletonName = std::string(pRootBone->mName.C_Str());

    auto exportPath = sceneContext.GetOutputDirectory() / skeletonName;
    exportPath.replace_extension("skel");

    RawSkeleton* pSkeleton = nullptr;
    if (!sceneContext.TryGetSkeleton(skeletonName, pSkeleton))
    {
        pSkeleton->m_name = skeletonName;
        pSkeleton->m_id = AssetID(exportPath.string());
        pSkeleton->m_rootNodeGlobalTransform = sceneContext.GetGlobalTransform(pRootBone);

        auto numBones = pSkeletalMesh->mNumBones + 1;
        std::vector<std::string> parentBoneNames;

        parentBoneNames.reserve(numBones);
        pSkeleton->m_boneNames.reserve(numBones);
        pSkeleton->m_parentBoneIndices.reserve(numBones);
        pSkeleton->m_globalReferencePose.reserve(numBones);
        pSkeleton->m_localReferencePose.reserve(numBones);

        // Add root data
        pSkeleton->m_localReferencePose.push_back(Transform::Identity);
        pSkeleton->m_globalReferencePose.push_back(Transform::Identity);
        pSkeleton->m_boneNames.push_back(skeletonName);
        parentBoneNames.push_back("");

        // Initialize the rest of the bones
        for (size_t meshBoneIndex = 0; meshBoneIndex < pSkeletalMesh->mNumBones; ++meshBoneIndex)
        {
            auto pBone = pSkeletalMesh->mBones[meshBoneIndex];
            auto boneName = std::string(pBone->mName.C_Str());

            auto parentBoneName = std::string(pBone->mNode->mParent->mName.C_Str());
            parentBoneNames.push_back(parentBoneName);

            // TODO: Choose the right one !
            auto inverseBindPose = sceneContext.DecomposeMatrix(pBone->mOffsetMatrix);
            auto bindPose = Transform(glm::inverse(sceneContext.ToGLM(pBone->mOffsetMatrix)));

            // Add bone data
            pSkeleton->m_boneNames.push_back(boneName);
            pSkeleton->m_globalReferencePose.push_back(bindPose);
        }

        // Find parent bone indices
        pSkeleton->m_parentBoneIndices.push_back(InvalidIndex);
        for (size_t boneIndex = 1; boneIndex < numBones; ++boneIndex)
        {
            auto parentBoneIndex = pSkeleton->GetBoneIndex(parentBoneNames[boneIndex]);
            pSkeleton->m_parentBoneIndices.push_back(parentBoneIndex);
        }

        pSkeleton->CalculateLocalTransforms();

        pSkeleton->SortBones();

        // Save skeleton
        std::vector<std::byte> data;
        auto dataStream = BinaryMemoryArchive(data, IBinaryArchive::IOMode::Write);

        pSkeleton->Serialize(dataStream);

        // TODO: Use skeleton static asset type
        auto header = AssetArchiveHeader("skel");
        auto archive = BinaryFileArchive(pSkeleton->m_id.GetAssetPath(), IBinaryArchive::IOMode::Write);
        archive << header << data;
    }

    assert(pSkeleton != nullptr);
    return pSkeleton;
}
} // namespace aln::assets::converter