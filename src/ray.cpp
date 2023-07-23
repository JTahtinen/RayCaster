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

RayContent getContentFromEdge(jadel::Vec2 rayPos, int xDir, int yDir, uint32 clip, const Map *map)
{
    if (rayPos.x <= 0 || rayPos.y <= 0 || rayPos.x >= map->width || rayPos.y >= map->height)
    {
        RayContent result;
        result.type = RAY_CONTENT_TYPE_NULL;
        return result;
    }
    int xContent;
    int yContent;

    float distanceFromLeftEdge = 0;

    if (clip & HORIZONTAL_CLIP)
    {
        if (xDir > 0)
        {
            xContent = jadel::roundToInt(rayPos.x);
            distanceFromLeftEdge = 1.0f - (rayPos.y - (float)((int)rayPos.y));
        }
        else if (xDir <= 0)
        {
            xContent = jadel::roundToInt(rayPos.x) - 1;
            distanceFromLeftEdge = rayPos.y - (float)((int)rayPos.y);
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
            distanceFromLeftEdge = rayPos.x - (float)((int)rayPos.x);
        }
        else if (yDir <= 0)
        {
            yContent = jadel::roundToInt(rayPos.y) - 1;
            distanceFromLeftEdge = 1.0f - (rayPos.x - (float)((int)rayPos.x));
        }
        
    }
    else
    {
        yContent = (int)rayPos.y;
    }
    const Sector *mapUnit = map->getSectorContent(xContent, yContent);
    RayContent result;
    result.type = RAY_CONTENT_TYPE_SECTOR;
    result.texture = mapUnit->texture;
    result.sectorX = xContent;
    result.sectorY = yContent;
    result.color = mapUnit->color;
    result.barrier = mapUnit->barrier;
    result.isVerticallyClipped = (clip & VERTICAL_CLIP);
    result.isClippedOnCorner = (clip & VERTICAL_CLIP) && (clip & HORIZONTAL_CLIP);
    result.distanceFromLeftEdge = distanceFromLeftEdge;
    return result;
}

void setRayResult(Ray *ray, jadel::Vec2 start, jadel::Vec2 end, ClipArray clips)
{
    ray->start = start;
    ray->end = end;
    ray->clips = clips;
}

int Ray::shoot(const jadel::Vec2 start, const jadel::Vec2 dir, float maxDistance, const Map *map)
{
    if (maxDistance < 0)
        return 0;
    int xDir = dir.x > 0 ? 1 : (dir.x == 0 ? 0 : -1);
    int yDir = dir.y > 0 ? 1 : (dir.y == 0 ? 0 : -1);
    int isXDirPositive = xDir != -1 ? 1 : 0;
    int isYDirPositive = yDir != -1 ? 1 : 0;
    jadel::Vec2 dirNormalized = dir.normalize();
    bool startXAtEdge = isFloatEven(start.x, 0.01f);
    bool startYAtEdge = isFloatEven(start.y, 0.01f);
    ClipArray clipArray;
    if (startXAtEdge || startYAtEdge)
    {
        uint32 clipFlags = (VERTICAL_CLIP * (uint32)startYAtEdge) | (HORIZONTAL_CLIP * (uint32)startXAtEdge);
        // jadel::message("ClipFlags: %d\n", (int)clipFlags);
        const RayContent rayStartContent = getContentFromEdge(start, xDir, yDir, clipFlags, map);
        clipArray.push({jadel::Vec2(0, 0), rayStartContent});
        if (rayStartContent.barrier)
        {
            setRayResult(this, start, start, clipArray);
            return 1;
        }
    }
    if (xDir == 0 && yDir == 0)
    {
        RayClip nullClip;
        nullClip.content.type = RAY_CONTENT_TYPE_NULL;
        this->clips.push(nullClip);
        return 0;
    }

    const Sector *rayEndContent = Sector::getNullMapUnit();

    bool raySegmentIsVerticallyClipped = false;
    bool raySegmentIsClippedOnCorner = false;
    jadel::Vec2 rayMarchPos = start;
    int numDebugFloats = 0;
    jadel::Vec2 rayDest = start + dirNormalized * maxDistance;
    bool rayXReached = false;
    bool rayYReached = false;

    int safeGuardCounter = 0;
    jadel::Vec2 remainingRay = rayDest - rayMarchPos;
    while (isVec2ALongerThanB(remainingRay, rayMarchPos - start))
    {

        float rayEndDistToHorizontalSector = (float)((int)(rayMarchPos.x + (float)isXDirPositive)) - rayMarchPos.x;
        float rayEndDistToVerticalSector = (float)((int)(rayMarchPos.y + (float)isYDirPositive)) - rayMarchPos.y;

        if (!isYDirPositive && IS_VAL_BETWEEN_INCLUSIVE(rayEndDistToVerticalSector, 0, 0.0000001f))
        {
            rayEndDistToVerticalSector = -1.0f;
        }
        if (!isXDirPositive && IS_VAL_BETWEEN_INCLUSIVE(rayEndDistToHorizontalSector, 0, 0.0000001f))
        {
            rayEndDistToHorizontalSector = -1.0f;
        }

        jadel::Vec2 clippedRaySegment(0, 0);

        if (fabs(remainingRay.x) < fabs(rayEndDistToHorizontalSector))
        {
            rayXReached = true;
        }
        if (fabs(remainingRay.y) < fabs(rayEndDistToVerticalSector))
        {
            rayYReached = true;
        }
        if (rayXReached && rayYReached)
        {
            rayMarchPos += remainingRay;
            this->end = rayDest;
            break;
        }
        else
        {
            /*
            Cut a segment from a line whose slope is the same as the ray's,
            and whose length covers the distance to the next sector.
            */

            jadel::Vec2 horizontallyClippedRaySegment = clipSegmentFromLineAtX(dirNormalized, rayEndDistToHorizontalSector);
            jadel::Vec2 verticallyClippedRaySegment = clipSegmentFromLineAtY(dirNormalized, rayEndDistToVerticalSector);

            /*
                Compare the length of the ray that clips with the next sector in the X-axis
                with the one that clips with the Y-axis, choose the shorter one, and add it
                to the existing ray
            */

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
                if (compareVec2(horizontallyClippedRaySegment, verticallyClippedRaySegment, 0.00001f))
                {
                    clippedRaySegment = verticallyClippedRaySegment;
                    raySegmentIsVerticallyClipped = true;
                    raySegmentIsClippedOnCorner = true;
                }
                else if (isVec2ALongerThanB(horizontallyClippedRaySegment, verticallyClippedRaySegment))
                {
                    clippedRaySegment = verticallyClippedRaySegment;
                    raySegmentIsVerticallyClipped = true;
                    raySegmentIsClippedOnCorner = false;
                }
                else
                {
                    clippedRaySegment = horizontallyClippedRaySegment;
                    raySegmentIsVerticallyClipped = false;
                    raySegmentIsClippedOnCorner = false;
                }
                
            }
        }
        rayMarchPos += clippedRaySegment;

        uint32 clipFlags = HORIZONTAL_CLIP * (!raySegmentIsVerticallyClipped || raySegmentIsClippedOnCorner) | VERTICAL_CLIP * (raySegmentIsVerticallyClipped);
    
        // bool xMarginNeeded = ((!isXDirPositive && !raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);
        // bool yMarginNeeded = ((!isYDirPositive && raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);

        // float xMargin = xMarginNeeded * (-0.1f);
        // float yMargin = yMarginNeeded * (-0.1f);
        RayContent rayEndContent = getContentFromEdge(rayMarchPos, xDir, yDir, clipFlags, map);
        if (rayEndContent.type == RAY_CONTENT_TYPE_NULL || rayEndContent.barrier)
        {
            clipArray.push({rayMarchPos - start, rayEndContent});
            break;
        }
        remainingRay = rayDest - rayMarchPos;
        ++safeGuardCounter;
        if (safeGuardCounter == MAX_CLIPS)
        {
            break;
        }
    }
    setRayResult(this, start, rayMarchPos, clipArray);
    if (this->clips.empty())
    {
        RayClip nullClip;
        nullClip.content.type = RAY_CONTENT_TYPE_NULL;
        this->clips.push(nullClip);
    }
    return 1;
}

RayClip Ray::getRayEndClip() const
{
    RayClip result = this->clips.back();
    return result;
}