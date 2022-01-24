#pragma once

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
    uint32_t GetNumWeights() const { return boneWeights.size(); }
    float GetBoneWeight(uint32_t boneIdx) const { return boneWeights.at(boneIdx); }
};
} // namespace aln