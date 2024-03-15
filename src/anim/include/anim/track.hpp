#pragma once

#include "track_key.hpp"

#include <common/containers/vector.hpp>
#include <common/transform.hpp>
#include <common/types.hpp>

#include <string>

namespace aln
{

class FrameTime;

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
    Transform Sample(const FrameTime& frameTime) const;

  private:
    // TODO: Compress !!
    // TrackCompressionSettings m_compressionSettings;
    Vector<Transform> m_transforms;
};
} // namespace aln