#pragma once

#include "types.hpp"

#include <string>
#include <vector>

namespace aln
{

class Bone
{
    friend class SkeletonLoader;

  private:
    BoneIndex m_index = InvalidIndex;
    BoneIndex m_parentIndex = InvalidIndex;
    std::string m_name = "";

  public:
    inline const std::string& GetName() const { return m_name; }
    inline BoneIndex GetIndex() const { return m_index; }
    inline BoneIndex GetParentIndex() const { return m_parentIndex; }
    inline const std::string& GetName() { return m_name; }

    inline void SetParent(BoneIndex parentIndex) { m_parentIndex = parentIndex; }
    inline bool IsRootBone() const { return m_parentIndex == InvalidIndex; }
};
} // namespace aln