#pragma once

#include "../task.hpp"

#include <string>

namespace aln
{

class AnimationClip;

class SampleTask : public Task
{
  private:
    const AnimationClip* m_pAnimation;
    Percentage m_time;

  public:
    SampleTask(NodeIndex sourceNodeIdx, const AnimationClip* pAnimation, Percentage time);

    void Execute(const TaskContext& context) override;

#ifndef NDEBUG
    std::string GetDebugText() const
    {
    }
#endif
};
} // namespace aln