#pragma once

#include "assimp_animation.hpp"

#include <assets/asset_system/animation_clip_asset.hpp>
#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/skeleton_asset.hpp>

#include <iostream>

namespace aln::assets::converter
{

aiNodeAnim* AssimpAnimationReader::GetChannel(const aiAnimation* pAnimation, const std::string& boneName)
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

void AssimpAnimationReader::ReadAnimation(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const AssimpSkeleton* pSkeleton)
{
    assert(pSkeleton->HasPathOnDisk());

    AnimationClipInfo animInfo;
    animInfo.originalFile = sceneContext.GetSourceFile().string();
    animInfo.name = std::string(pAnimation->mName.C_Str());
    animInfo.duration = pAnimation->mDuration;
    animInfo.framesPerSecond = pAnimation->mTicksPerSecond;
    animInfo.skeletonID = pSkeleton->GetPathOnDisk();

    size_t currentIndex = 0;

    // Process animation tracks
    std::vector<float> animData;
    auto boneCount = pSkeleton->GetBoneCount();
    animInfo.tracks.reserve(boneCount);
    for (auto boneIndex = 0; boneIndex < boneCount; ++boneIndex)
    {
        auto boneName = pSkeleton->GetBoneName(boneIndex);

        // TODO: tmp. Add empty keys to root joint
        if (boneIndex == 0)
        {
            TrackInfo trackInfo;
            trackInfo.boneName = boneName; // TODO: TMP
            trackInfo.numTranslationKeys = 2;
            trackInfo.numRotationKeys = 2;
            trackInfo.numScaleKeys = 2;

            trackInfo.indexInBuffer = 0;
            auto lastFrame = (float) animInfo.duration;

            std::vector<float> rootData;
            // auto t = pSkeleton->GetRootBoneGlobalTransform();
            auto t = Transform::Identity;
            const auto& rootTranslation = t.GetTranslation();
            const auto& rootRotation = t.GetRotation();
            const auto& rootScale = t.GetScale();

            rootData.push_back(0.0f);
            rootData.push_back(rootTranslation.x);
            rootData.push_back(rootTranslation.y);
            rootData.push_back(rootTranslation.z);

            rootData.push_back(lastFrame);
            rootData.push_back(rootTranslation.x);
            rootData.push_back(rootTranslation.y);
            rootData.push_back(rootTranslation.z);

            rootData.push_back(0.0f);
            rootData.push_back(rootRotation.x);
            rootData.push_back(rootRotation.y);
            rootData.push_back(rootRotation.z);
            rootData.push_back(rootRotation.w);

            rootData.push_back(lastFrame);
            rootData.push_back(rootRotation.x);
            rootData.push_back(rootRotation.y);
            rootData.push_back(rootRotation.z);
            rootData.push_back(rootRotation.w);

            rootData.push_back(0.0f);
            rootData.push_back(rootScale.x);
            rootData.push_back(rootScale.y);
            rootData.push_back(rootScale.z);

            rootData.push_back(lastFrame);
            rootData.push_back(rootScale.x);
            rootData.push_back(rootScale.y);
            rootData.push_back(rootScale.z);

            animData.insert(animData.end(), rootData.begin(), rootData.end());
            currentIndex += animData.size() * sizeof(float);

            animInfo.tracks.push_back(trackInfo);
        }

        auto pChannel = GetChannel(pAnimation, boneName);
        if (pChannel != nullptr)
        {
            TrackInfo trackInfo;
            trackInfo.boneName = boneName;
            trackInfo.numTranslationKeys = pChannel->mNumPositionKeys;
            trackInfo.numRotationKeys = pChannel->mNumRotationKeys;
            trackInfo.numScaleKeys = pChannel->mNumScalingKeys;

            trackInfo.indexInBuffer = currentIndex;

            // Increment index in buffer for the next track
            currentIndex += trackInfo.GetBufferValuesCount();

            animData.reserve(animData.size() + trackInfo.GetBufferValuesCount());
            for (size_t keyIndex = 0; keyIndex < trackInfo.numTranslationKeys; keyIndex++)
            {
                auto& key = pChannel->mPositionKeys[keyIndex];

                animData.push_back((float) key.mTime);

                auto translation = key.mValue;
                animData.push_back((float) translation.x);
                animData.push_back((float) translation.y);
                animData.push_back((float) translation.z);
            }

            for (size_t keyIndex = 0; keyIndex < trackInfo.numRotationKeys; keyIndex++)
            {
                auto& key = pChannel->mRotationKeys[keyIndex];

                animData.push_back((float) key.mTime);

                auto rotation = key.mValue;
                animData.push_back((float) rotation.x);
                animData.push_back((float) rotation.y);
                animData.push_back((float) rotation.z);
                animData.push_back((float) rotation.w);
            }

            for (size_t keyIndex = 0; keyIndex < trackInfo.numScaleKeys; keyIndex++)
            {
                auto& key = pChannel->mScalingKeys[keyIndex];

                animData.push_back((float) key.mTime);

                auto scale = key.mValue;
                animData.push_back((float) scale.x);
                animData.push_back((float) scale.y);
                animData.push_back((float) scale.z);
            }

            // TODO: Handle Assimp pre and post state

            animInfo.tracks.push_back(trackInfo);
        }
        else
        {
            // No channel for this bone in the animation.
            /// @todo : Log somewhere clever, maybe the class itself ?
            std::cout << "Warning: no channel for bone " << boneName << std::endl;
            // TODO: Insert data from the skeleton's reference pose
        }
    }

    AssetFile file = PackAnimationClip(&animInfo, animData);

    std::string animName = pAnimation->mName.C_Str();
    if (animName == "")
    {
        animName = "Default";
    }
    // std::replace(animName.begin(), animName.end(), '|', '_');
    auto path = (sceneContext.GetOutputDirectory() / animName);
    // TODO: Use the asset static type info
    path.replace_extension(".anim");

    SaveBinaryFile(path.string(), file);
}
} // namespace aln::assets::converter