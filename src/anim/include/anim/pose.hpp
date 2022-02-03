#pragma once

#include <vector>

#include "bone.hpp"

#include <common/transform.hpp>

namespace aln
{

class Skeleton;

/// @brief Shamelessly taken from https://www.youtube.com/watch?v=Jkv0pbp0ckQ&t=1697s
/// @brief Container for the transforms of all bones in a skeleton
class Pose
{
    friend class Blender;

  public:
    enum class InitialState : uint8_t
    {
        None,
        ReferencePose,
        ZeroPose,
    };

    enum class State : uint8_t
    {
        Unset,
        Pose,
        ReferencePose,
        ZeroPose,
        AdditivePose,
    };

  private:
    const Skeleton* m_pSkeleton = nullptr;
    std::vector<Transform> m_localTransforms;  // Parent-space transforms
    std::vector<Transform> m_globalTransforms; // Character-space transforms
    State m_state = State::Unset;

  public:
    Pose(const Skeleton* pSkeleton, InitialState initialState = InitialState::ReferencePose);

    // Move
    Pose(Pose&& rhs);
    Pose& operator=(Pose&& rhs);

    // Disable copy operations to prevent accidental copies
    Pose(const Pose& rhs) = delete;
    Pose& operator=(const Pose& rhs) = delete;

    void CopyFrom(const Pose& rhs);
    inline void CopyFrom(const Pose* pRhs) { CopyFrom(*pRhs); }

    // Pose state
    void Reset(InitialState initState = InitialState::None, bool calcGlobalPose = false);

    inline bool IsPoseSet() const { return m_state != State::Unset; }
    inline bool IsReferencePose() const { return m_state == State::ReferencePose; }
    inline bool IsZeroPose() const { return m_state == State::ZeroPose; }
    inline bool IsAdditivePose() const { return m_state == State::AdditivePose; }

    // Global Transform Cache
    inline bool HasGlobalTransforms() const { return !m_globalTransforms.empty(); }
    inline void ClearGlobalTransforms() { m_globalTransforms.clear(); }
    inline const std::vector<Transform>& GetGlobalTransforms() const { return m_globalTransforms; }
    void CalculateGlobalTransforms();
    Transform GetGlobalTransform(BoneIndex boneIdx) const;

    // Getters
    inline const Skeleton* GetSkeleton() const { return m_pSkeleton; }
    size_t GetNumBones() const { return m_localTransforms.size(); };

    // Local Transforms
    Transform GetTransform(BoneIndex boneIdx);

    void SetTransform(BoneIndex boneIdx, const Transform& transform);
    void SetTranslation(BoneIndex boneIdx, const glm::vec3& translation);
    void SetScale(BoneIndex boneIdx, const glm::vec3& scale);
    void SetRotation(BoneIndex boneIdx, const glm::quat& rotation);
};
} // namespace aln