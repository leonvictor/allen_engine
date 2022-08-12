#pragma once

#include "asset_system.hpp"
#include <common/transform.hpp>

namespace aln::assets
{

struct BoneInfo
{
    std::string name;
    uint32_t parentIndex;
};

struct SkeletonInfo
{
    std::vector<std::string> boneNames;
    std::vector<uint32_t> boneParentIndices;
    CompressionMode compressionMode;

    std::string originalFilePath = "";
    std::string assetPath = "";

    bool operator==(const SkeletonInfo& other) const
    {
        auto boneCount = boneNames.size();
        if (boneCount != other.boneNames.size())
        {
            return false;
        }
        for (auto idx = 0; idx < boneCount; ++idx)
        {
            if (boneNames[idx] != other.boneNames[idx])
            {
                return false;
            }
        }
        return true;
    }
};

struct SkeletonConverter
{
    /// @brief Read a skeleton's AssetFile and populate its SkeletonInfo
    static SkeletonInfo ReadInfo(AssetFile* file);
    static void Unpack(const SkeletonInfo* info, const std::vector<std::byte>& srcBuffer, std::byte* dstBuffer);
    static AssetFile Pack(SkeletonInfo* info, std::vector<Transform>& referencePose);
};
} // namespace aln::assets