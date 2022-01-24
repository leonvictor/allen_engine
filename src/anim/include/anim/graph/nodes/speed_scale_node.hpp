#pragma once

#include "../graph_context.hpp"
#include "../passthrough_node.hpp"

#include <common/math.hpp>

namespace aln
{
class SpeedScaleNode : public PassthroughNode
{
  private:
    float m_blendWeight = 1.0f;
    ValueNode* m_pScaleValueNode = nullptr;

    // TODO
    float GetSpeedScale(GraphContext& context);

  public:
    struct Settings : public PassthroughNode
    {
        float m_blendTime;
    };

    PoseNodeResult Update(GraphContext& context) override
    {
        // Record old delta time
        auto const deltaTime = context.m_deltaTime;

        // Adjust the delta time for the child node
        if (IsChildValid())
        {
            auto speedScale = 1.0f;
            if (m_pScaleValueNode != nullptr)
            {
                speedScale = GetSpeedScale(context);

                if (m_blendWeight < 1.0f)
                {
                    auto pSettings = GetSettings<SpeedScaleNode>();
                    assert(pSettings->m_blendTime >= 0.0f);
                    const float blendWeightDeltaTime = context.m_deltaTime / pSettings->m_blendTime;
                    m_blendWeight = Math::Clamp(m_blendWeight + blendWeightDeltaTime, 0.0f, 1.0f);
                    speedScale = Math::Lerp(1.0f, speedScale, m_blendWeight);
                }

                context.m_deltaTime *= speedScale;
                m_duration = m_pChildNode->GetDuration() / speedScale;
            }
        }

        // Update the child node
        PoseNodeResult result = PassthroughNode::Update(context);

        // Reset the delta time
        context.m_deltaTime = deltaTime;
        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        // TODO: Log warning: animation -> attempting to run a speed scale node in a synchronized manner, invalid op
        m_blendWeight = (Math::IsNearZero(GetSettings<SpeedScaleNode>()->m_blendTime)) ? 1.0f : 0.0f;
        return PassthroughNode::Update(context, updateRange);
    }
};

} // namespace aln
