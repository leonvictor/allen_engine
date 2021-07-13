#pragma once

#include "entities/entity.hpp"
#include "entities/world_entity.cpp"

class Scene
{
    WorldEntity worldEntity;

    Scene()
    {
        Entity cube = Entity();
    }
};