#pragma once

#include "pose.hpp"
#include "skeleton.hpp"

namespace aln
{

Pose::Pose(const Skeleton* pSkeleton, InitialState initialState)
    : m_pSkeleton(pSkeleton)
{
    assert(pSkeleton != nullptr);
    Reset(initialState, false);
}

Pose::Pose(Pose&& rhs)
{
    m_pSkeleton = std::move(rhs.m_pSkeleton);
    m_state = std::move(rhs.m_state);
    m_localTransforms = std::move(rhs.m_localTransforms);
    m_globalTransforms = std::move(rhs.m_globalTransforms);
}

Pose& Pose::operator=(Pose&& rhs)
{
    m_pSkeleton = std::move(rhs.m_pSkeleton);
    m_state = std::move(rhs.m_state);
    m_localTransforms = std::move(rhs.m_localTransforms);
    m_globalTransforms = std::move(rhs.m_globalTransforms);
    return *this;
}

void Pose::CopyFrom(const Pose& rhs)
{
    m_pSkeleton = rhs.m_pSkeleton;
    m_state = rhs.m_state;
    m_localTransforms = rhs.m_localTransforms;
    m_globalTransforms = rhs.m_globalTransforms;
}

void Pose::Reset(InitialState initState, bool calcGlobalPose)
{
    // TODO: What's the behavior for transforms at initialization ?
    switch (initState)
    {
    case InitialState::None:
    {
        m_globalTransforms.clear();
        m_localTransforms.clear();

        m_globalTransforms.resize(m_pSkeleton->GetBonesCount());
        m_localTransforms.resize(m_pSkeleton->GetBonesCount());

        m_state = State::Unset;
    }
    break;
    case InitialState::ReferencePose:
    {
        // TODO: Set all transforms to the bone's reference
        // TODO: Copy here or condition in getters ? (if m_state == State::ReferencePose) {...}
        m_localTransforms = m_pSkeleton->GetLocalReferencePose();
        m_globalTransforms = m_pSkeleton->GetGlobalReferencePose();
        m_state = State::ReferencePose;
    }
    break;
    case InitialState::ZeroPose:
    {
        // TODO: What is a "zero pose" ?
        m_state = State::ZeroPose;
    }
    break;
    }

    if (calcGlobalPose)
    {
        CalculateGlobalTransforms();
    }
    else
    {
        // TODO:
        // m_globalTransforms.reserve(m_pSkeleton->GetBonesCount());
    }
}

void Pose::CalculateGlobalTransforms()
{
    const auto boneCount = m_pSkeleton->GetBonesCount();
    m_globalTransforms.resize(boneCount);

    m_globalTransforms[0] = m_localTransforms[0];
    for (auto boneIdx = 1; boneIdx < boneCount; boneIdx++)
    {
        const auto parentIdx = m_pSkeleton->GetParentBoneIndex(boneIdx);

        assert(parentIdx < boneIdx);

        m_globalTransforms[boneIdx] = m_globalTransforms[parentIdx] * m_localTransforms[boneIdx];
    }
}

Transform Pose::GetGlobalTransform(BoneIndex boneIdx) const
{
    assert(boneIdx < m_pSkeleton->GetBonesCount() && boneIdx < m_globalTransforms.size());
    return m_globalTransforms[boneIdx];
}

Transform Pose::GetTransform(BoneIndex boneIdx)
{
    assert(boneIdx < m_pSkeleton->GetBonesCount() && boneIdx < m_localTransforms.size());
    return m_localTransforms[boneIdx];
}

void Pose::SetTransform(BoneIndex boneIdx, const Transform& transform)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx] = transform;
}

void Pose::SetTranslation(BoneIndex boneIdx, const glm::vec3& translation)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx].SetTranslation(translation);
}

void Pose::SetScale(BoneIndex boneIdx, const glm::vec3& scale)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx].SetScale(scale);
}

void Pose::SetRotation(BoneIndex boneIdx, const glm::quat& rotation)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx].SetRotation(rotation);
}
} // namespace aln