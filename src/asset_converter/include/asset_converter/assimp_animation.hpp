#pragma once

#include "assimp_scene_context.hpp"
#include "assimp_skeleton.hpp"

namespace aln::assets::converter
{
/// @todo : Should be approx. the same as the runtime AnimationClip class
/// @todo : Better conversion to disk format
class AssimpAnimation
{
    friend class AssimpAnimationReader;

  public:
    struct TrackData
    {
        /// @todo
    };

  private:
    AssimpSkeleton m_skeleton;

  public:
    AssimpAnimation(const AssimpSkeleton& skeleton) : m_skeleton(skeleton) {}
};

class AssimpAnimationReader
{
  private:
    static aiNodeAnim* GetChannel(const aiAnimation* pAnimation, const std::string& boneName);

  public:
    static void ReadAnimation(const AssimpSceneContext& sceneContext, const aiAnimation* pAnimation, const AssimpSkeleton* pSkeleton);
};
} // namespace aln::assets::converter