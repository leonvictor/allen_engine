#pragma once

#include "bone_mask.hpp"
#include "pose.hpp"
#include "skeleton.hpp"

#include <assert.h>

#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

namespace aln
{

template <typename T>
class BitFlags
{
    // TODO
};

enum class PoseBlend
{
    // TODO
};

struct InterpolativeBlender
{
    inline static glm::quat BlendRotation(const glm::quat& quat0, const glm::quat& quat1, const float t)
    {
        return glm::slerp(quat0, quat1, t);
    }

    inline static glm::vec3 BlendTranslation(const glm::vec3& trans0, const glm::vec3& trans1, const float t)
    {
        return glm::lerp(trans0, trans1, t);
    }

    inline static glm::vec3 BlendScale(const glm::vec3& scale0, const glm::vec3& scale1, const float t)
    {
        return glm::lerp(scale0, scale1, t);
    }
};

struct AdditiveBlender
{
    inline static glm::quat BlendRotation(const glm::quat& quat0, const glm::quat& quat1, const float t)
    {
        const glm::quat targetQuat = quat0 * quat1;
        return glm::slerp(quat0, targetQuat, t);
    }

    inline static glm::vec3 BlendTranslation(const glm::vec3& trans0, const glm::vec3& trans1, const float t)
    {
        // TODO
        // return Vector::MultiplyAdd(trans1, Vector(t), trans0).SetW1();
    }

    inline static glm::vec3 BlendScale(const glm::vec3& scale0, const glm::vec3& scale1, const float t)
    {
        // TODO (same as translation (multiplyadd))
    }
};

/// @todo : Do we actually want to use another type than float as weights ?
struct BlendWeight
{
    /// @brief
    inline static float GetBlendWeight(float blendWeight, const BoneMask* pBoneMask, uint32_t boneIdx)
    {
        if (pBoneMask != nullptr)
        {
            blendWeight *= pBoneMask->GetBoneWeight(boneIdx);
        }
        return blendWeight;
    }
};

/// @brief Blend between two poses in local bone space
/// @tparam Blender: Blender type
/// @tparam BlendWeight: TODO
/// @param pSourcePose:
/// @param pTargetPose:
/// @param blendWeight: 0 = source pose, 1 = target pose
/// @param pBoneMask:
/// @param pResultPose:
template <typename Blender>
void BlenderLocal(const Pose* pSourcePose, const Pose* pTargetPose, const float blendWeight, const BoneMask* pBoneMask, Pose* pResultPose)
{
    assert(blendWeight >= 0.0f && blendWeight <= 1.0f);
    assert(pSourcePose != nullptr && pTargetPose != nullptr && pResultPose != nullptr);

    if (pBoneMask != nullptr)
    {
        assert(pBoneMask->GetNumWeights() == pSourcePose->GetSkeleton()->GetBonesCount());
    }

    const uint32_t numBones = pResultPose->GetBonesCount();
    for (uint32_t boneIdx = 0; boneIdx < numBones; boneIdx++)
    {
        // If the bone has been masked out
        float const boneBlendWeight = BlendWeight::GetBlendWeight(blendWeight, pBoneMask, boneIdx);
        if (boneBlendWeight == 0.0f)
        {
            pResultPose->SetTransform(boneIdx, pSourcePose->GetTransform(boneIdx));
        }
        else // Perform blend
        {
            const Transform& sourceTransform = pSourcePose->GetTransform(boneIdx);
            const Transform& targetTransform = pTargetPose->GetTransform(boneIdx);

            // Blend translations
            const glm::vec3 translation = Blender::BlendTranslation(sourceTransform.GetTranslation(), targetTransform.GetTranslation(), boneBlendWeight);
            pResultPose->SetTranslation(boneIdx, translation);

            const glm::vec3 scale = Blender::BlendScale(sourceTransform.GetScale(), targetTransform.GetScale(), boneBlendWeight);
            pResultPose->SetScale(boneIdx, scale);

            const glm::quat rotation = Blender::BlendRotation(sourceTransform.GetRotation(), targetTransform.GetRotation(), boneBlendWeight);
            pResultPose->SetRotation(boneIdx, rotation);
        }
    }
}

struct Blender
{
    /// @brief Perform a standard interpolative blend
    /// @todo Perform a different blend based on options. The bitmask might be a bit much ?
    static void Blend(const Pose* pSourcePose, const Pose* pTargetPose, const float blendWeight, BitFlags<PoseBlend> blendOptions, const BoneMask* pBoneMask, Pose* pResultPose)
    {
        BlenderLocal<InterpolativeBlender>(pSourcePose, pTargetPose, blendWeight, pBoneMask, pResultPose);
    }
};
} // namespace aln