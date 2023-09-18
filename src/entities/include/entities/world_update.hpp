#pragma once

#include <common/update_stages.hpp>
#include <common/containers/hash_map.hpp>

#include <assert.h>

namespace aln
{
/// @brief Represents the stages during which a system should be updated, as well as the system's priority in each stage.
struct UpdatePriorities
{
    HashMap<UpdateStage, uint8_t> m_updatePriorityMap;

    /// @brief Whether the provided stage is enabled.
    bool IsUpdateStageEnabled(const UpdateStage& stage) const
    {
        return m_updatePriorityMap.count(stage) == 1;
    }

    /// @brief Returns the priority for the provided stage. Making sure the stage is enabled is the user's responsibility.
    uint8_t GetPriorityForStage(const UpdateStage& stage) const
    {
        assert(IsUpdateStageEnabled(stage));
        return m_updatePriorityMap.at(stage);
    }

    void SetPriorityForStage(const UpdateStage& stage, uint8_t priority)
    {
        m_updatePriorityMap.insert({stage, priority});
    }

    // TODO: allow systems to add and (maybe) update their priorities
};
}