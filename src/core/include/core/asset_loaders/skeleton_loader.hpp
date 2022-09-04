#pragma once

#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/skeleton_asset.hpp>
#include <assets/loader.hpp>

#include <anim/skeleton.hpp>

#include <common/types.hpp>

#include <memory>
#include <vector>

namespace aln
{

class SkeletonLoader : public IAssetLoader
{
  private:
    bool Load(AssetRecord* pRecord, const assets::AssetFile& file) override
    {
        assert(pRecord->IsUnloaded());
        assert(file.m_assetTypeID == Skeleton::GetStaticAssetTypeID());

        Skeleton* pSkeleton = aln::New<Skeleton>();

        auto info = assets::SkeletonConverter::ReadInfo(&file);

        auto boneCount = info.boneNames.size();
        pSkeleton->m_boneNames.resize(boneCount);
        pSkeleton->m_parentBoneIndices.resize(boneCount);
        for (BoneIndex idx = 0; idx < boneCount; ++idx)
        {
            pSkeleton->m_boneNames[idx] = info.boneNames[idx];
            pSkeleton->m_parentBoneIndices[idx] = info.boneParentIndices[idx];
        }

        pSkeleton->m_localReferencePose.resize(boneCount);
        assets::SkeletonConverter::Unpack(&info, file.m_binary, (std::byte*) pSkeleton->m_localReferencePose.data());

        // Calculate global pose
        pSkeleton->m_globalReferencePose.resize(boneCount);
        pSkeleton->m_globalReferencePose[0] = pSkeleton->m_localReferencePose[0];
        for (BoneIndex boneIdx = 1; boneIdx < boneCount; boneIdx++)
        {
            const auto parentIdx = pSkeleton->GetParentBoneIndex(boneIdx);

            assert(parentIdx < boneIdx);

            pSkeleton->m_globalReferencePose[boneIdx] = pSkeleton->m_globalReferencePose[parentIdx] * pSkeleton->m_localReferencePose[boneIdx];
        }

        pRecord->SetAsset(pSkeleton);
        return true;
    }
};

} // namespace aln