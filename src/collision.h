#pragma once
#include "rc.h"

struct CollisionData
{
    bool xCollision;
    bool yCollision;
    jadel::Vec2 checkedVelocity;

    bool check(Map* map, jadel::Vec2 position, jadel::Vec2 velocity);
};