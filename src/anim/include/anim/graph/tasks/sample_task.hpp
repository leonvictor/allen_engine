#pragma once

#include "../../animation_clip.hpp"
#include "../task.hpp"

namespace aln
{
class SampleTask : public Task
{
  private:
    AnimationClip const* m_pAnimation;
    Percentage m_time;

  public:
    SampleTask(NodeIndex sourceNodeIdx, const AnimationClip* pAnimation, Percentage time) : Task(sourceNodeIdx), m_pAnimation(pAnimation), m_time(time)
    {
        assert(m_pAnimation != nullptr);
    }

    void Execute(const TaskContext& context) override
    {
        assert(m_pAnimation != nullptr);

        auto pResultBuffer = GetNewPoseBuffer(context);
        m_pAnimation->GetPose(m_time, &pResultBuffer->m_pose);
        MarkTaskComplete(context);
    }

#ifndef NDEBUG
    std::string GetDebugText() const
    {
    }
#endif
};
} // namespace aln