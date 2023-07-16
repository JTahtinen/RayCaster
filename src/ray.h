#pragma once
#include <jadel.h>

struct Map;
struct MapUnit;
struct Ray
{
    jadel::Vec2 start;
    jadel::Vec2 end;
    bool isVerticallyClipped;
    bool isClippedOnCorner;
    const MapUnit* rayEndContent;

    float length() const;
    jadel::Vec2 rayVector() const;
    int shoot(const jadel::Vec2 start, const jadel::Vec2 dir, uint32 maxDistance, const Map* map);
};