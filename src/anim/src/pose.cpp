#pragma once

#include "pose.hpp"

namespace aln
{

Pose::Pose(const Skeleton* pSkeleton, InitialState initialState = InitialState::ReferencePose)
    : m_pSkeleton(pSkeleton), m_state(initialState)
{
    // TODO: What's the behavior for transforms at initialization ?
    m_localTransforms.resize(pSkeleton->GetNumBones());
    m_globalTransforms.reserve(pSkeleton->GetNumBones());
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
}

void Pose::CopyFrom(const Pose& rhs)
{
    m_pSkeleton = rhs.m_pSkeleton;
    m_state = rhs.m_state;
    m_localTransforms = rhs.m_localTransforms;
    m_globalTransforms = rhs.m_globalTransforms;
}

void Pose::Reset(InitialState initState = InitialState::None, bool calcGlobalPose = false)
{
    m_state = initState;
    // TODO: What's the behavior for transforms at initialization ?
    m_localTransforms.clear();
    m_localTransforms.resize(m_pSkeleton->GetNumBones());
    m_globalTransforms.clear();
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