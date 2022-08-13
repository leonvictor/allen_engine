#pragma once

#include "types.hpp"

#include <vector>

namespace aln
{
/// @brief Associate a weight to each of a skeleton's bones. Used to partially blend two poses for example.
/// @todo Bone mask editor
class BoneMask
{
  private:
    std::vector<float> boneWeights;
    // Based on animation skeleton

  public:
    size_t GetNumWeights() const { return boneWeights.size(); }
    float GetBoneWeight(BoneIndex boneIdx) const { return boneWeights.at(boneIdx); }
};
} // namespace aln