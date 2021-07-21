#pragma once

#include <vector>

namespace aln::entities
{

class Entity;

// TODO: Move somewhere where this makes sense
class Command
{
  private:
  public:
    // TODO: Visibility ?
    std::vector<Entity*> m_updatedEntities;
    void Execute(Entity* pEntity);
};
} // namespace aln::entities