#include "ray.h"
#include "util.h"
#include "rc.h"
#include <math.h>

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

int Ray::shoot(const jadel::Vec2 start, const jadel::Vec2 dir, uint32 maxDistance, const Map* map)
{
    if (maxDistance < 1)
        return 0;
    int isXDirPositive = dir.x > 0 ? 1 : 0;
    int isYDirPositive = dir.y > 0 ? 1 : 0;

    // jadel::Vec2 unRotatedScreenClip = clipSegmentFromLineAtY(unRotatedRayLine, defaultScreenPlane.y);
    // jadel::Vec2 rotatedScreenClip = cameraRotationMatrix.mul(unRotatedScreenClip);

    const MapUnit* rayEndContent = MapUnit::getNullMapUnit();

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
            clippedRaySegment = horizontallyClippedRaySegment;
            raySegmentIsClippedOnCorner = true;
        }
        rayMarchPos += clippedRaySegment;
        if (rayMarchPos.x <= 0 || rayMarchPos.y <= 0 || rayMarchPos.x >= map->height || rayMarchPos.y >= map->height)
            break;

        bool xMarginNeeded = ((!isXDirPositive && !raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);
        bool yMarginNeeded = ((!isYDirPositive && raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);

        float xMargin = xMarginNeeded * (-0.1f);
        float yMargin = yMarginNeeded * (-0.1f);

        rayEndContent = map->getSectorContent(floorf(rayMarchPos.x + xMargin), floorf(rayMarchPos.y + yMargin));
        if (rayEndContent->barrier)
        {
            break;
        }
    }
    this->start = start;
    this->end = rayMarchPos;
    this->isVerticallyClipped = raySegmentIsVerticallyClipped;
    this->isClippedOnCorner = raySegmentIsClippedOnCorner;
    this->rayEndContent = rayEndContent;
    return 1;
}