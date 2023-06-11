#pragma once

#include <assets/asset_archive_header.hpp>
#include <assets/asset_id.hpp>
#include <common/serialization/binary_archive.hpp>

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"
#include "raw_skeleton.hpp"

#include <iostream>
#include <vector>

namespace aln::assets::converter
{
class RawAnimation : public IRawAsset
{
    friend class AssimpAnimationReader;

    struct TrackData
    {
        std::vector<Transform> m_transforms;
    };

    std::vector<TrackData> m_tracks;
    std::vector<Transform> m_rootMotionTrack;

    std::string m_name;
    float m_duration; // Duration in seconds
    float m_framesPerSecond;

    AssetID m_skeletonID;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        archive << m_duration;
        archive << m_framesPerSecond;

        // TODO: Compress

        archive << m_tracks.size();
        for (auto& track : m_tracks)
        {
            archive << track.m_transforms;
        }

        archive << m_rootMotionTrack;
    }
};

struct AssimpAnimationReader
{
    static aiNodeAnim* GetChannel(const aiAnimation* pAnimation, const std::string& boneName)
    {
        for (auto channelIndex = 0; channelIndex < pAnimation->mNumChannels; ++channelIndex)
        {
            auto pChannel = pAnimation->mChannels[channelIndex];
            if (std::string(pChannel->mNodeName.C_Str()) == boneName)
            {
                return pChannel;
            }
        }

        return nullptr;
    }

    static void ReadAnimation(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const RawSkeleton* pSkeleton)
    {
        assert(pAnimation != nullptr && pSkeleton != nullptr);
        assert(pSkeleton->GetID().IsValid());
        assert(pSkeleton->GetBonesCount() >= pAnimation->mNumChannels);

        RawAnimation animation;
        animation.m_name = sceneContext.GetSourceFile().stem().string() + "_" + std::string(pAnimation->mName.C_Str());
        animation.m_duration = pAnimation->mDuration / pAnimation->mTicksPerSecond;
        animation.m_skeletonID = pSkeleton->GetID();

        uint32_t maxKeyCount = 0;

        // Process animation tracks
        auto boneCount = pSkeleton->GetBonesCount();
        animation.m_tracks.reserve(boneCount);

        for (auto boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            auto boneName = pSkeleton->GetBoneName(boneIndex);
            auto& track = animation.m_tracks.emplace_back();

            auto pChannel = GetChannel(pAnimation, boneName);
            if (pChannel != nullptr)
            {
                /// @note For now we only support animations where tracks have full transforms for each frame
                // AND the frames are uniformely spaced
                // (compression should happen later)
                assert(pChannel->mNumPositionKeys == pChannel->mNumRotationKeys);

                bool uniformScaling = (pChannel->mNumScalingKeys == 1);
                assert(uniformScaling || pChannel->mNumRotationKeys == pChannel->mNumScalingKeys);

                const auto keyCount = pChannel->mNumPositionKeys;
                maxKeyCount = std::max(maxKeyCount, keyCount);

                track.m_transforms.reserve(pChannel->mNumPositionKeys);
                for (auto keyIndex = 0; keyIndex < keyCount; ++keyIndex)
                {
                    auto& translation = pChannel->mPositionKeys[keyIndex].mValue;
                    auto& rotation = pChannel->mRotationKeys[keyIndex].mValue;
                    auto& scale = uniformScaling ? pChannel->mScalingKeys[0].mValue : pChannel->mScalingKeys[keyIndex].mValue;

                    auto& transform = track.m_transforms.emplace_back(
                        sceneContext.ToGLM(translation),
                        sceneContext.ToGLM(rotation),
                        sceneContext.ToGLM(scale));
                }
            }
            else
            {
                /// @todo : Log somewhere clever, maybe the class itself ?
                std::cout << "Warning: no channel for bone " << boneName << std::endl;
                // TODO: Insert data from the skeleton's reference pose
                track.m_transforms.push_back(Transform::Identity);
            }
        }

        animation.m_framesPerSecond = animation.m_duration / (maxKeyCount - 1);

        // Extract root motion to a separate track and replace it with an empty one
        auto& rootMotionTrack = animation.m_tracks[0];
        auto rootMotionFrameCount = rootMotionTrack.m_transforms.size();
        animation.m_rootMotionTrack.reserve(rootMotionFrameCount);
        for (auto frameIdx = 0; frameIdx < rootMotionFrameCount; ++frameIdx)
        {
            animation.m_rootMotionTrack.push_back(rootMotionTrack.m_transforms[frameIdx]);
            // TODO: TMP while we do not handle root motion : remove translation but keep everything else
            //rootMotionTrack.m_transforms[frameIdx] = Transform::Identity;
            rootMotionTrack.m_transforms[frameIdx].SetTranslation(glm::vec3());
        }

        std::vector<std::byte> data;
        BinaryMemoryArchive dataStream(data, IBinaryArchive::IOMode::Write);
        animation.Serialize(dataStream);

        AssetArchiveHeader header("anim"); // TODO: Use SkeletalMesh::GetStaticAssetType();
        header.AddDependency(pSkeleton->GetID());

        auto path = (sceneContext.GetOutputDirectory() / (animation.m_name + ".anim"));
        auto archive = BinaryFileArchive(path.string(), IBinaryArchive::IOMode::Write);
        archive << header << data;
    }
};
} // namespace aln::assets::converter
