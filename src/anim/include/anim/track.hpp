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
    Transform Sample(uint32_t frameIndex, float frameProgress) const;

  private:
    // TODO: Compress !!
    // TrackCompressionSettings m_compressionSettings;
    std::vector<Transform> m_transforms;
};
} // namespace aln