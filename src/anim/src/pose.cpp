#pragma once

#include "pose.hpp"

namespace aln
{

Pose::Pose(const Skeleton* pSkeleton, InitialState initialState)
    : m_pSkeleton(pSkeleton)
{
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
        m_localTransforms.resize(m_pSkeleton->GetNumBones());
        m_state = State::Unset;
    }
    break;
    case InitialState::ReferencePose:
    {
        // TODO: Set all transforms to the bone's reference
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
        m_globalTransforms.reserve(m_pSkeleton->GetNumBones());
    }
}

void Pose::CalculateGlobalTransforms()
{
    // TODO
}

Transform Pose::GetGlobalTransforms(uint32_t boneIdx) const
{
    assert(boneIdx < m_pSkeleton->GetNumBones());
    assert(boneIdx < m_globalTransforms.size());
    return m_globalTransforms[boneIdx];
}

Transform Pose::GetTransform(uint32_t boneIdx)
{
    assert(boneIdx < m_pSkeleton->GetNumBones());
    assert(boneIdx < m_localTransforms.size());
    return m_localTransforms[boneIdx];
}

void Pose::SetTransform(uint32_t boneIdx, const Transform& transform)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx] = transform;
}

void Pose::SetTranslation(uint8_t boneIdx, const glm::vec3& translation)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx].SetTranslation(translation);
}

void Pose::SetScale(uint8_t boneIdx, const glm::vec3& scale)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx].SetScale(scale);
}

void Pose::SetRotation(uint8_t boneIdx, const glm::quat& rotation)
{
    assert(boneIdx < m_localTransforms.size());
    m_localTransforms[boneIdx].SetRotation(rotation);
}
} // namespace aln