#include <set>
#include "common.cpp"

namespace ecs
{
class System
{
public:
    std::set<Entity> mEntities;
};
} // namespace ecs
