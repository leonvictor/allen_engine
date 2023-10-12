#pragma once

#include <assets/asset_archive_header.hpp>
#include <assets/asset_id.hpp>
#include <common/serialization/binary_archive.hpp>

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"
#include "raw_skeleton.hpp"

#include <common/containers/vector.hpp>

#include <iostream>

namespace aln::assets::converter
{
class RawAnimation : public IRawAsset
{
    friend class AssimpAnimationReader;

    struct TrackData
    {
        Vector<Transform> m_transforms;
    };

    Vector<TrackData> m_tracks;
    Vector<Transform> m_rootMotionTrack;

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
        assert(pAnimation != nullptr);

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
                maxKeyCount = Maths::Max(maxKeyCount, keyCount);

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

        // Extract root motion to a separate track
        auto& rootBoneTrack = animation.m_tracks[0];
        auto rootMotionStartOffset = rootBoneTrack.m_transforms[0].GetTranslation();
        rootMotionStartOffset.y = 0; // TMP: See below

        auto rootMotionFrameCount = rootBoneTrack.m_transforms.size();
        animation.m_rootMotionTrack.reserve(rootMotionFrameCount);
        for (auto frameIdx = 0; frameIdx < rootMotionFrameCount; ++frameIdx)
        {
            auto& rootBoneTransform = rootBoneTrack.m_transforms[frameIdx];
            rootBoneTransform.AddTranslation(-rootMotionStartOffset);

            // TMP fix: Root bone keeps its vertical component. Necessary for mixamo character whose root bone is the hips.
            // Not ideal: will not work for vertical root motion, i.e. jumps
            // Ideally we would let the user control the import process from the editor and pick whether to adjust or not
            auto rootMotionTransform = rootBoneTransform;
            rootMotionTransform.SetTranslation(rootMotionTransform.GetTranslation() * Vec3(1.0f, 0.0f, 1.0f));
            rootMotionTransform.SetRotation(Quaternion::Identity);
            animation.m_rootMotionTrack.push_back(rootMotionTransform);

            rootBoneTransform.SetTranslation(rootBoneTransform.GetTranslation() * Vec3::Y);
        }

        // --- Serialization
        Vector<std::byte> data;
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
