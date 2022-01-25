#pragma once

#include <string>
#include <vector>

#include "track_key.hpp"

#include <common/transform.hpp>

#include <glm/ext/quaternion_common.hpp>
#include <glm/gtx/compatibility.hpp>

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
    size_t GetNumKeys() const { return m_keys.size(); }

    Transform Sample(float time) const
    {
        auto it = m_keys.begin();
        while (it->m_time <= time)
        {
            ++it;
        }
        const TrackKey& before = *it;
        const TrackKey& after = *(++it);

        // TODO: Sample
        auto t = time / (after.m_time - before.m_time); // TODO !!
        auto& beforeTransform = before.m_component;
        auto& afterTransform = after.m_component;

        Transform result;
        result.SetTranslation(glm::lerp(beforeTransform.GetTranslation(), afterTransform.GetTranslation(), t));
        result.SetScale(glm::lerp(beforeTransform.GetScale(), afterTransform.GetScale(), t));
        result.SetRotation(glm::slerp(beforeTransform.GetRotation(), afterTransform.GetRotation(), t));

        // TODO: Handle loop behavior

        return result;

    } // TODO

  private:
    std::string m_boneName;
    std::vector<TrackKey> m_keys;

    // TODO: Compress !!
    // TrackCompressionSettings m_compressionSettings;
};
} // namespace aln