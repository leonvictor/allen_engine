#pragma once

#include <assets/loader.hpp>

namespace aln
{

class SkeletonLoader : public IAssetLoader
{
  private:
    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
};
} // namespace aln