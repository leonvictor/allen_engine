#include "world_system.cpp"
#include <vector>
/// @brief The entity that will hold all the world systems.
class WorldEntity
{
  private:
    std::vector<WorldSystem> systems;
};