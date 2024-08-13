#include "asset_loaders/skeleton_loader.hpp"

#include <anim/skeleton.hpp>
#include <common/types.hpp>

namespace aln
{
bool SkeletonLoader::Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive)
{
    assert(pRecord->IsUnloaded());
    assert(pRecord->GetAssetTypeID() == Skeleton::GetStaticAssetTypeID());

    Skeleton* pSkeleton = aln::New<Skeleton>();

    archive >> pSkeleton->m_boneNames;
    archive >> pSkeleton->m_parentBoneIndices;
    archive >> pSkeleton->m_globalReferencePose;
    archive >> pSkeleton->m_localReferencePose;

    // Calculate global pose
    // pSkeleton->m_globalReferencePose.resize(boneCount);
    // pSkeleton->m_globalReferencePose[0] = pSkeleton->m_localReferencePose[0];
    // for (BoneIndex boneIdx = 1; boneIdx < boneCount; boneIdx++)
    // {
    //     const auto parentIdx = pSkeleton->GetParentBoneIndex(boneIdx);

    //     assert(parentIdx < boneIdx);

    //     pSkeleton->m_globalReferencePose[boneIdx] = pSkeleton->m_globalReferencePose[parentIdx] * pSkeleton->m_localReferencePose[boneIdx];
    // }

    pRecord->SetAsset(pSkeleton);
    return true;
}
} // namespace aln