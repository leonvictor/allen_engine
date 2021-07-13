#pragma once

#include "../update_stages.hpp"
#include <assert.h>
#include <unordered_map>

namespace aln::entities
{
/// @brief Represents the stages during which a system should be updated, as well as the system's priority in each stage.
struct UpdatePriorities
{
    std::unordered_map<UpdateStage, uint8_t> m_updatePriorityMap;

    /// @brief Whether the provided stage is enabled.
    bool IsUpdateStageEnabled(const UpdateStage& stage) const
    {
        return m_updatePriorityMap.count(stage) == 1;
    }

    /// @brief Returns the priority for the provided stage. Making sure the stage is enabled is the user's responsibility.
    uint8_t GetPriorityForStage(const UpdateStage& stage)
    {
        assert(IsUpdateStageEnabled(stage));
        return m_updatePriorityMap[stage];
    }

    // TODO: allow systems to add and (maybe) update their priorities
};
}