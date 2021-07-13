#pragma once

namespace aln::entities
{

class Entity;

// TODO: Move somewhere where this makes sense
class Command
{
  public:
    void Execute(Entity* pEntity);
};
} // namespace aln::entities