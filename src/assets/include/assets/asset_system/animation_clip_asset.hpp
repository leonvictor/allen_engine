#pragma once

#include <cstdint>
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
    std::string boneName;
    uint32_t numKeys;
};

struct AnimationClipInfo
{
    std::string name;         // Clip name
    uint32_t duration;        // Clip duration (in frames)
    uint16_t framesPerSecond; // Source clip FPS

    // uint32_t numTracks;       // Number of animation tracks
    std::vector<TrackInfo> tracks;

    size_t bufferSize;                     // Uncompressed buffer size
    CompressionMode binaryCompressionMode; // Compression mode of the binary blob
    // AnimCompressionMode animCompressionMode; // Compression mdoe of the data
    std::string originalFile;
};

/// @brief Read the json metadata and convert it in a TextInfo struct.
AnimationClipInfo ReadAnimationClipInfo(AssetFile* file);

/// @brief Decompress an animation into a buffer.
/// @param info: Clip info
/// @param sourceBuffer Binary compressed animation data
/// @param destinationBuffer TODO: Directly in the engine Animation class ?
void UnpackAnimationClip(const AnimationClipInfo* info, const std::vector<std::byte>& sourceBuffer, std::vector<float>& destinationBuffer);

/// @brief Compress and store an animation clip into binary format.
/// @param info:
AssetFile PackAnimationClip(AnimationClipInfo* info, std::vector<float>& data);

// TextureFormat ParseFormat(std::string formatString);
} // namespace aln::assets