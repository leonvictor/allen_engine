#include "track.hpp"

#include "frame_time.hpp"

#include <common/types.hpp>

namespace aln
{

Transform Track::Sample(const FrameTime& frameTime) const
{
    /// @todo TMP fix while we wait for a proper anim sampling system
    if (m_transforms.size() == 1)
    {
        return m_transforms[0];
    }

    const auto frameIdx = frameTime.GetFrameIndex();
    const auto percentageThroughFrame = frameTime.GetPercentageThroughFrame();

    assert(frameIdx < m_transforms.size());
    assert(percentageThroughFrame <= 1.0f && percentageThroughFrame >= 0.0f);

    auto& frameTransform = m_transforms[frameIdx];
    auto& nextFrameTransform = m_transforms[frameIdx + 1];

    return Transform::Interpolate(frameTransform, nextFrameTransform, percentageThroughFrame);
}

} // namespace aln