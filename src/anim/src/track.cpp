#include "track.hpp"

#include <common/types.hpp>

#include <glm/ext/quaternion_common.hpp>
#include <glm/gtx/compatibility.hpp>

namespace aln
{

// TODO: Rework track storage and sampling, this is terrible
Transform Track::Sample(float animationTime) const
{
    size_t frameIndex;
    size_t nextFrameIndex;

    // --------------
    // Find Translation
    // ---------------

    glm::vec3 translation;

    // Look for the frame right after this time
    frameIndex = FindNextFrameIndex(m_translationKeys, animationTime);
    nextFrameIndex = frameIndex + 1;

    assert(nextFrameIndex < m_translationKeys.size());

    {
        auto& before = m_translationKeys[frameIndex];
        auto& after = m_translationKeys[nextFrameIndex];

        auto factor = (animationTime - before.m_time) / (after.m_time - before.m_time);
        translation = glm::lerp(before.m_component, after.m_component, factor);
    }

    // --------------
    // Find Rotation
    // ---------------

    glm::quat rotation;
    frameIndex = FindNextFrameIndex(m_rotationKeys, animationTime);
    nextFrameIndex = frameIndex + 1;

    assert(nextFrameIndex < m_rotationKeys.size());

    {
        auto& before = m_rotationKeys[frameIndex];
        auto& after = m_rotationKeys[nextFrameIndex];

        auto factor = (animationTime - before.m_time) / (after.m_time - before.m_time);
        rotation = glm::normalize(glm::slerp(before.m_component, after.m_component, factor));
    }

    // --------------
    // Find Scale
    // ---------------

    glm::vec3 scale;
    frameIndex = FindNextFrameIndex(m_scaleKeys, animationTime);
    nextFrameIndex = frameIndex + 1;

    assert(nextFrameIndex < m_scaleKeys.size());

    {
        auto& before = m_scaleKeys[frameIndex];
        auto& after = m_scaleKeys[nextFrameIndex];

        auto factor = (animationTime - before.m_time) / (after.m_time - before.m_time);
        scale = glm::lerp(before.m_component, after.m_component, factor);
    }

    Transform result(translation, rotation, scale);
    return result;
}

} // namespace aln