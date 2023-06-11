#pragma once

#include <assert.h>

#include <glm/gtc/epsilon.hpp>

#include "../../blender.hpp"
#include "../../bone_mask.hpp"
#include "../task.hpp"

namespace aln
{

class BlendTask : public Task
{
  private:
    float m_blendWeight = 1.0f;
    const BoneMask* m_pBoneMask;
    BitFlags<PoseBlend> m_blendOptions;

  public:
    BlendTask(NodeIndex sourceNodeIdx, TaskIndex sourceTaskIndex, TaskIndex targetTaskIndex, float const blendWeight, BitFlags<PoseBlend> blendOptions, const BoneMask* pBoneMask)
        : Task(sourceNodeIdx, UpdateStage::Any, {sourceTaskIndex, targetTaskIndex}),
          m_blendWeight(blendWeight),
          m_pBoneMask(pBoneMask),
          m_blendOptions(blendOptions)
    {
        assert(m_blendWeight >= 0.0f && m_blendWeight <= 1.0f);

        // TODO: Use a standard epsilon
        if (glm::epsilonEqual(m_blendWeight, 1.0f, 0.000001f))
        {
            m_blendWeight = 1.0f;
        }
    }

    void Execute(const TaskContext& context)
    {
        auto pSourceBuffer = TransferDependencyPoseBuffer(context, 0);
        auto pTargetBuffer = AccessDependencyPoseBuffer(context, 1);
        auto pFinalBuffer = pSourceBuffer;

        Blender::Blend(&pSourceBuffer->m_pose, &pTargetBuffer->m_pose, m_blendWeight, m_blendOptions, m_pBoneMask, &pFinalBuffer->m_pose);

        ReleaseDependencyPoseBuffer(context, 1);
        MarkTaskComplete(context);
    }

#ifndef NDEBUG
    std::string GetDebugText() const
    {
    }
#endif
};
} // namespace aln