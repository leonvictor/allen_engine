/// Prototype of the world system responsible for managing entities.
#include "world_system.cpp"
#include "world_update.hpp"

class EntityManager : IWorldSystem
{
    void Update()
    {
        // TODO: Does not belong here
        // From SO: How can I iterate over an enum? (https://stackoverflow.com/questions/261963/)
        for (int updateInt = UpdateStage::FrameStart; updateInt != UpdateStage::NumStages; updateInt++)
        {
            auto stage = static_cast<UpdateStage>(updateInt);

            // TODO: Spatial dependencies: parent entities are scheduled to update before their children
            // TODO: Parallelize

            // TODO: Update EntitySystems
            // then
            // TODO: Update World Systems
        }
    }
};