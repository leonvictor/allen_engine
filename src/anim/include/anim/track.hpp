#pragma once

#include <string>
#include <vector>

#include "track_key.hpp"

#include <common/transform.hpp>
#include <common/types.hpp>

namespace aln
{
// struct QuantizationRange
// {
//     float low;
//     float high;
// };

// /// @note https://takinginitiative.wordpress.com/2020/03/07/an-idiots-guide-to-animation-compression/
// struct TrackCompressionSettings
// {
//     friend class AnimationCompiler;

//   public:
//     inline bool IsTranslationTrackXStatic() const { return m_isTranslationStaticX; }
//     inline bool IsTranslationTrackYStatic() const { return m_isTranslationStaticY; }
//     inline bool IsTranslationTrackZStatic() const { return m_isTranslationStaticZ; }
//     inline bool IsScaleTrackStatic() const { return m_isScaleStatic; }

//   private:
//     bool m_isTranslationStaticX = false;
//     bool m_isTranslationStaticY = false;
//     bool m_isTranslationStaticZ = false;
//     bool m_isScaleStatic = false;

//     QuantizationRange m_translationRangeX;
//     QuantizationRange m_translationRangeY;
//     QuantizationRange m_translationRangeZ;
//     QuantizationRange m_scaleRange;
//     uint32_t m_trackStartIndex = 0; // The start offset for this tracks in the compressed data block (in number of UI16s)
// };

/// @brief Animation track, containing a sequence of TrackKeys
/// @todo Track contains **compressed** data
class Track
{
    friend class AnimationLoader;

  public:
    const std::string& GetBoneName() const { return m_boneName; }

    Transform Sample(float time) const;

  private:
    std::string m_boneName;

    std::vector<TrackKey<glm::vec3>> m_translationKeys;
    std::vector<TrackKey<glm::quat>> m_rotationKeys;
    std::vector<TrackKey<glm::vec3>> m_scaleKeys;

    // TODO: Compress !!
    // TrackCompressionSettings m_compressionSettings;

    /// @brief Find the next frame after a specified time in a track.
    /// If the time is before the first key, return 0, if it is after the last, return InvalidIndex
    template <typename T>
    uint32_t FindNextFrameIndex(const std::vector<TrackKey<T>>& track, float time) const
    {
        for (uint64_t i = 0; i < track.size() - 1; i++)
        {
            if (time < track[i + 1].m_time)
            {
                return i;
            }
        }
        assert(0);
    }
};
} // namespace aln