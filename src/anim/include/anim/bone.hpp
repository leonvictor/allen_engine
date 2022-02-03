#pragma once

#include "types.hpp"

#include <string>
#include <vector>

namespace aln
{

class Bone
{
  private:
    BoneIndex m_index = InvalidIndex;
    BoneIndex m_parentIndex = InvalidIndex;
    std::vector<BoneIndex> m_childrenIndices;
    std::string m_name;

  public:
    Bone(BoneIndex index, std::string name) : m_index(index), m_name(name) {}

    inline const std::string& GetName() const { return m_name; }
    inline BoneIndex GetIndex() const { return m_index; }
    inline BoneIndex GetParentIndex() const { return m_parentIndex; }
    inline const std::vector<BoneIndex>& GetChildrenIndices() const { return m_childrenIndices; }
    inline const std::string& GetName() { return m_name; }

    inline void SetParent(BoneIndex parentIndex) { m_parentIndex = parentIndex; }
    inline void AddChild(BoneIndex childIndex) { m_childrenIndices.push_back(childIndex); }

    inline bool HasChildren() const { return !m_childrenIndices.empty(); }
    inline bool IsRootBone() const { return m_parentIndex == InvalidIndex; }
};
} // namespace aln