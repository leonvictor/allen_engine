#pragma once

#include "../graph_context.hpp"
#include "../passthrough_node.hpp"
#include "../pose_node.hpp"
#include "../value_node.hpp"

#include <common/math.hpp>
#include <common/transform.hpp>

#include <glm/vec3.hpp>

namespace aln
{

enum class OverrideFlags
{
    HeadingX,
    HeadingY,
    HeadingZ,

    FacingX,
    FacingY,
    FacingZ,
};

// TODO: Generalize
// TODO: name ?
struct OverrideFlagsSet
{
    template <class... Flags>
    bool AreAnyFlagsSet(Flags... flags)
    {
        // TODO
    }

    bool IsFlagSet(OverrideFlags flag)
    {
        // TODO
    }
};

class RootMotionOverrideNode : public PassthroughRuntimeNode
{
  public:
    struct Settings : public RuntimeGraphNode::Settings
    {
        OverrideFlagsSet m_overrideFlags;
        float m_maxLinearVelocity;
    };

  private:
    ValueNode* m_pDesiredHeadingVelocityNode = nullptr;
    ValueNode* m_pDesiredFacingDirectionNode = nullptr;
    ValueNode* m_pLinearVelocityLimitNode = nullptr;

    void ModifyDisplacement(GraphContext& context, PoseNodeResult& nodeResult) const
    {
        auto pSettings = GetSettings<RootMotionOverrideNode>();
        assert(context.IsValid() && pSettings != nullptr);

        Transform adjustedDisplacementDelta = nodeResult.m_rootMotionDelta;

        // Heading
        bool isHeadingModificationAllowed = m_pDesiredHeadingVelocityNode != nullptr &&
                                            pSettings->m_overrideFlags.AreAnyFlagsSet(OverrideFlags::HeadingX, OverrideFlags::HeadingY, OverrideFlags::HeadingZ);

        float maxLinearVelocity = pSettings->m_maxLinearVelocity;
        if (m_pLinearVelocityLimitNode != nullptr)
        {
            maxLinearVelocity = Math::Abs(m_pLinearVelocityLimitNode->GetValue<float>(context) * 100);
            isHeadingModificationAllowed = !Math::IsNearZero(maxLinearVelocity);
        }

        if (isHeadingModificationAllowed)
        {
            const glm::vec3 desiredHeadingVelocity = m_pDesiredHeadingVelocityNode->GetValue<glm::vec3>(context);

            // Override the request axes with the desired heading
            glm::vec3 translation = nodeResult.m_rootMotionDelta.GetTranslation();
            translation.x = pSettings->m_overrideFlags.IsFlagSet(OverrideFlags::HeadingX) ? desiredHeadingVelocity.x * context.m_deltaTime : translation.x;
            translation.y = pSettings->m_overrideFlags.IsFlagSet(OverrideFlags::HeadingY) ? desiredHeadingVelocity.y * context.m_deltaTime : translation.y;
            translation.z = pSettings->m_overrideFlags.IsFlagSet(OverrideFlags::HeadingZ) ? desiredHeadingVelocity.z * context.m_deltaTime : translation.z;

            // Apply max linear velocity limit
            const float maxLinearValue = context.m_deltaTime * maxLinearVelocity;
            if (glm::length2(translation) > (maxLinearValue * maxLinearValue))
            {
                translation = glm::normalize(translation);
                translation *= maxLinearValue;
            }

            adjustedDisplacementDelta.SetTranslation(translation);
        }

        // Facing
        // TODO:

        bool isFacingModificationAllowed = (m_pDesiredFacingDirectionNode != nullptr) && pSettings->m_overrideFlags.AreAnyFlagsSet(OverrideFlags::FacingX, OverrideFlags::FacingY, OverrideFlags::FacingZ);
    }

    PoseNodeResult Update(GraphContext& context) override
    {
        PoseNodeResult result = PassthroughRuntimeNode::Update(context);
        ModifyDisplacement(context, result);
        return result;
    }

    PoseNodeResult Update(GraphContext& context, const SyncTrackTimeRange& updateRange) override
    {
        PoseNodeResult result = PassthroughRuntimeNode::Update(context, updateRange);
        ModifyDisplacement(context, result);
        return result;
    }
};
} // namespace aln