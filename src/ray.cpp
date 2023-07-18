#include "ray.h"
#include "util.h"
#include "rc.h"
#include <math.h>
#include <jadel.h>

float Ray::length() const
{
    float result = rayVector().length();
    return result;
}

jadel::Vec2 Ray::rayVector() const
{
    jadel::Vec2 result = end - start;
    return result;
}

enum Clip
{
    VERTICAL_CLIP = 1,
    HORIZONTAL_CLIP = 1 << 1,
};

const MapUnit *getContentFromEdge(Ray *ray, jadel::Vec2 rayPos, int xDir, int yDir, uint32 clip, const Map *map)
{
    int xContent;
    int yContent;
    
    if (clip & HORIZONTAL_CLIP)
    {
        if (xDir > 0)
        {
            xContent = jadel::roundToInt(rayPos.x);
        }
        else if (xDir <= 0)
        {
            xContent = jadel::roundToInt(rayPos.x) - 1;
        }
    }
    else
    {
        xContent = (int)rayPos.x;
    }
    if (clip & VERTICAL_CLIP)
    {
        if (yDir > 0)
        {
            yContent = jadel::roundToInt(rayPos.y);
        }
        else if (yDir <= 0)
        {
            yContent = jadel::roundToInt(rayPos.y) - 1;
        }
    }
    else
    {
        yContent = (int)rayPos.y;
    }
    const MapUnit *result = map->getSectorContent(xContent, yContent);

    return result;
}

void setRayResult(Ray *ray, jadel::Vec2 start, jadel::Vec2 end, bool isVerticallyClipped, bool isClippedOnCorner, const MapUnit *rayEndContent)
{
    ray->start = start;
    ray->end = end;
    ray->isVerticallyClipped = isVerticallyClipped;
    ray->isClippedOnCorner = isClippedOnCorner;
    ray->rayEndContent = rayEndContent;
}

int Ray::shoot(const jadel::Vec2 start, const jadel::Vec2 dir, uint32 maxDistance, const Map *map)
{
    if (maxDistance < 1)
        return 0;
    int isXDirPositive = dir.x >= 0 ? 1 : 0;
    int isYDirPositive = dir.y >= 0 ? 1 : 0;
    int xDir = dir.x > 0 ? 1 : (dir.x == 0 ? 0 : -1);
    int yDir = dir.y > 0 ? 1 : (dir.y == 0 ? 0 : -1);
    bool startXAtEdge = isFloatEvenWithinMargin(start.x, 0.001f);
    bool startYAtEdge = isFloatEvenWithinMargin(start.y, 0.001f);
    if (startXAtEdge || startYAtEdge)
    {
        uint32 clipFlags = (VERTICAL_CLIP * (uint32)startYAtEdge) | (HORIZONTAL_CLIP * (uint32)startXAtEdge);
        // jadel::message("ClipFlags: %d\n", (int)clipFlags);
        const MapUnit *rayStartContent = getContentFromEdge(this, start, xDir, yDir, clipFlags, map);
        if (rayStartContent->barrier)
        {
            bool isClippedOnCorner = startXAtEdge && startYAtEdge;
            static int i = 0;
            if (isClippedOnCorner)
                jadel::message("Corner collision %d!\n", i++);
            setRayResult(this, start, start, startYAtEdge, isClippedOnCorner, rayStartContent);
            return 1;
        }
        if (xDir == 0 && yDir == 0)
        {
            const MapUnit *content = map->getSectorContent(start.x, start.y);
            setRayResult(this, start, start, content->barrier, content->barrier, content);
            return 0;
        }
    }

    const MapUnit *rayEndContent = MapUnit::getNullMapUnit();

    bool raySegmentIsVerticallyClipped = false;
    bool raySegmentIsClippedOnCorner = false;
    jadel::Vec2 rayMarchPos = start;
    int numDebugFloats = 0;
    for (int i = 0; i < maxDistance; ++i)
    {
        float rayEndDistToHorizontalSector = (float)((int)(rayMarchPos.x + (float)isXDirPositive)) - rayMarchPos.x;
        float rayEndDistToVerticalSector = (float)((int)(rayMarchPos.y + (float)isYDirPositive)) - rayMarchPos.y;

        if (!isYDirPositive && rayEndDistToVerticalSector == 0)
        {
            rayEndDistToVerticalSector = -1.0f;
        }
        if (!isXDirPositive && rayEndDistToHorizontalSector == 0)
        {
            rayEndDistToHorizontalSector = -1.0f;
        }

        /*
        Cut a segment from a line whose slope is the same as the ray's,
        and whose length covers the distance to the next sector.
        */

        jadel::Vec2 horizontallyClippedRaySegment = clipSegmentFromLineAtX(dir, rayEndDistToHorizontalSector);
        jadel::Vec2 verticallyClippedRaySegment = clipSegmentFromLineAtY(dir, rayEndDistToVerticalSector);

        /*
            Compare the length of the ray that clips with the next sector in the X-axis
            with the one that clips with the Y-axis, choose the shorter one, and add it
            to the existing ray
        */

        jadel::Vec2 clippedRaySegment(0, 0);

        if (xDir == 0)
        {
            clippedRaySegment = verticallyClippedRaySegment;
            raySegmentIsVerticallyClipped = true;
            raySegmentIsClippedOnCorner = false;
        }
        else if (yDir == 0)
        {
            clippedRaySegment = horizontallyClippedRaySegment;
            raySegmentIsVerticallyClipped = false;
            raySegmentIsClippedOnCorner = false;
        }
        else
        {
            if (isVec2ALongerThanB(horizontallyClippedRaySegment, verticallyClippedRaySegment))
            {
                clippedRaySegment = verticallyClippedRaySegment;
                raySegmentIsVerticallyClipped = true;
                raySegmentIsClippedOnCorner = false;
            }
            else if (isVec2ALongerThanB(verticallyClippedRaySegment, horizontallyClippedRaySegment))
            {
                clippedRaySegment = horizontallyClippedRaySegment;
                raySegmentIsVerticallyClipped = false;
                raySegmentIsClippedOnCorner = false;
            }
            else
            {
                clippedRaySegment = verticallyClippedRaySegment;
                raySegmentIsVerticallyClipped = true;
                raySegmentIsClippedOnCorner = true;
            }
        }
        rayMarchPos += clippedRaySegment;
        if (rayMarchPos.x <= 0 || rayMarchPos.y <= 0 || rayMarchPos.x >= map->width || rayMarchPos.y >= map->height)
            break;

        uint32 clipFlags = 0;
        clipFlags += HORIZONTAL_CLIP * ((!raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner) | VERTICAL_CLIP * (raySegmentIsVerticallyClipped || raySegmentIsClippedOnCorner);
        // bool xMarginNeeded = ((!isXDirPositive && !raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);
        // bool yMarginNeeded = ((!isYDirPositive && raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);

        // float xMargin = xMarginNeeded * (-0.1f);
        // float yMargin = yMarginNeeded * (-0.1f);

        rayEndContent = getContentFromEdge(this, rayMarchPos, xDir, yDir, clipFlags, map); // map->getSectorContent(floorf(rayMarchPos.x + xMargin), floorf(rayMarchPos.y + yMargin));
        if (rayEndContent->barrier)
        {
            break;
        }
    }
    setRayResult(this, start, rayMarchPos, raySegmentIsVerticallyClipped, raySegmentIsClippedOnCorner, rayEndContent);
    return 1;
}