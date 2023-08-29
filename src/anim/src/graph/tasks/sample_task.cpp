#include "graph/tasks/sample_task.hpp"

#include "animation_clip.hpp"

namespace aln
{
SampleTask::SampleTask(NodeIndex sourceNodeIdx, const AnimationClip* pAnimation, Percentage time)
    : Task(sourceNodeIdx), m_pAnimation(pAnimation), m_time(time)
{
    assert(m_pAnimation != nullptr);
}

void SampleTask::Execute(const TaskContext& context) {
    assert(m_pAnimation != nullptr);

    auto pResultBuffer = GetNewPoseBuffer(context);
    // TODO: Having a dedicated Percentage class could allow us to directly use pAnimation->GetPose(percent)
    m_pAnimation->GetPose(m_time * m_pAnimation->GetDuration(), &pResultBuffer->m_pose);

    MarkTaskComplete(context);
}
}