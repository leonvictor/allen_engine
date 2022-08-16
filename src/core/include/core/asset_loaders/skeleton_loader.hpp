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
        pSkeleton->m_bones.resize(boneCount);
        for (BoneIndex idx = 0; idx < boneCount; ++idx)
        {
            auto& bone = pSkeleton->m_bones[idx];
            bone.m_index = idx;
            bone.m_parentIndex = (BoneIndex) info.boneParentIndices[idx];
            bone.m_name = info.boneNames[idx];

            // TODO: Fix root bone not found because of mismatching types
            if (bone.m_parentIndex == InvalidIndex)
            {
                pSkeleton->m_rootBone = &bone;
            }
        }

        pSkeleton->m_localReferencePose.resize(boneCount);
        assets::SkeletonConverter::Unpack(&info, file.m_binary, (std::byte*) pSkeleton->m_localReferencePose.data());

        // Calculate global pose
        pSkeleton->m_globalReferencePose.resize(boneCount);
        pSkeleton->m_globalReferencePose[0] = pSkeleton->m_localReferencePose[0];
        for (BoneIndex boneIdx = 1; boneIdx < boneCount; boneIdx++)
        {
            const auto pBone = pSkeleton->GetBone(boneIdx);
            const auto parentIdx = pBone->GetParentIndex();

            assert(parentIdx < boneIdx);

            pSkeleton->m_globalReferencePose[boneIdx] = pSkeleton->m_globalReferencePose[parentIdx] * pSkeleton->m_localReferencePose[boneIdx];
        }

        pRecord->SetAsset(pSkeleton);
        return true;
    }
};

} // namespace aln