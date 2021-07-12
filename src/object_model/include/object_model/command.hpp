#pragma once

class Entity;

// TODO: Move somewhere where this makes sense
class Command
{
  public:
    void Execute(Entity* pEntity);
};