#pragma once

#include "object_model/entity.hpp"
#include "object_model/world_entity.cpp"

class Scene
{
    WorldEntity worldEntity;

    Scene()
    {
        Entity cube = Entity();
    }
};