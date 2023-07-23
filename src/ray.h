#pragma once
#include <jadel.h>
#include "stackarray.h"
#include "texture.h"

struct Map;
struct Sector;

#define MAX_CLIPS (40)


enum RayContentType
{
    RAY_CONTENT_TYPE_NULL,
    RAY_CONTENT_TYPE_SECTOR,
    RAY_CONTENT_TYPE_ENTITY
};

struct RayContent
{
    RayContentType type;
    const Texture* texture;
    int sectorX;
    int sectorY;
    jadel::Color color;
    bool barrier;
    bool isVerticallyClipped;
    bool isClippedOnCorner;
    float distanceFromLeftEdge;
};

struct RayClip
{
    jadel::Vec2 distFromStart;
    RayContent content;
};

typedef jadel::StackArray<RayClip, 40> ClipArray;

struct Ray
{
    jadel::Vec2 start;
    jadel::Vec2 end;
    ClipArray clips;
    
    float length() const;
    jadel::Vec2 rayVector() const;
    int shoot(const jadel::Vec2 start, const jadel::Vec2 dir, float maxDistance, const Map* map);
    RayClip getRayEndClip() const;
};