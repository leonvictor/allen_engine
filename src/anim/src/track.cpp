#include "track.hpp"

#include <common/types.hpp>

namespace aln
{

Transform Track::Sample(uint32_t frameIndex, float frameProgress) const
{
    /// @todo TMP fix while we wait for a proper anim sampling system
    if (m_transforms.size() == 1)
    {
        return m_transforms[0];
    }

    assert(frameIndex < m_transforms.size());
    assert(frameProgress <= 1.0f && frameProgress >= 0.0f);

    auto& frameTransform = m_transforms[frameIndex];
    auto& nextFrameTransform = m_transforms[frameIndex + 1];

    return Transform::Interpolate(frameTransform, nextFrameTransform, frameProgress);
}

} // namespace aln