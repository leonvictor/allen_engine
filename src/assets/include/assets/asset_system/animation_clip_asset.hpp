#pragma once

#include <cstdint>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

#include <string>

#include "asset_system.hpp"

namespace aln::assets
{

enum class AnimCompressionMode : uint8_t
{
    None,
    Custom,
};

struct TrackInfo
{
    std::string boneName = "";

    uint64_t indexInBuffer = 0;

    uint32_t numTranslationKeys = 0;
    uint32_t numRotationKeys = 0;
    uint32_t numScaleKeys = 0;

    uint64_t GetTranslationBinaryBufferOffset() { return indexInBuffer; }
    uint64_t GetTranslationBinaryBufferSize() { return numTranslationKeys * 4 * sizeof(float); }

    uint64_t GetRotationBinaryBufferOffset() { return indexInBuffer + GetTranslationBinaryBufferSize(); }
    uint64_t GetRotationBinaryBufferSize() { return numRotationKeys * 5 * sizeof(float); }

    uint64_t GetScaleBinaryBufferOffset() { return indexInBuffer + GetTranslationBinaryBufferOffset() + GetRotationBinaryBufferOffset(); }
    uint64_t GetScaleBinaryBufferSize() { return numScaleKeys * 4 * sizeof(float); }

    /// @brief Return the number of float values used by this track
    uint64_t GetBufferValuesCount() { return (numTranslationKeys * 4) + (numRotationKeys * 5) + (numScaleKeys * 4); }
};

struct AnimationClipInfo
{
    std::string name;         // Clip name
    uint32_t duration;        // Clip duration (in frames)
    uint16_t framesPerSecond; // Source clip FPS

    // Associated skeleton asset ID
    std::string skeletonID;

    std::vector<TrackInfo> tracks;

    size_t binaryBufferSize;               // Uncompressed buffer size
    CompressionMode binaryCompressionMode; // Compression mode of the binary blob
    // AnimCompressionMode animCompressionMode; // Compression mdoe of the data
    std::string originalFile;
};

/// @brief Read the json metadata and convert it in a TextInfo struct.
AnimationClipInfo ReadAnimationClipInfo(const AssetFile* file);

/// @brief Decompress an animation into a buffer.
/// @param info: Clip info
/// @param sourceBuffer Binary compressed animation data
/// @param destinationBuffer TODO: Directly in the engine Animation class ?
void UnpackAnimationClip(const AnimationClipInfo* info, const std::vector<std::byte>& sourceBuffer, std::vector<float>& destinationBuffer);

/// @brief Compress and store an animation clip into binary format.
/// @param info:
AssetFile PackAnimationClip(const AnimationClipInfo* info, std::vector<float>& data);

// TextureFormat ParseFormat(std::string formatString);
} // namespace aln::assets