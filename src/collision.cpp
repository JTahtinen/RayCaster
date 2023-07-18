#include "collision.h"
#include "ray.h"
#include <math.h>
#include "util.h"

bool CollisionData::check(Map *map, jadel::Vec2 position, jadel::Vec2 velocity)
{
    this->xCollision = false;
    this->yCollision = false;
    float actorRadius = 0.3f;
    this->checkedVelocity = velocity;
    float velLength = velocity.length();
    if (velLength == 0)
    {
        return false;
    }

    Ray upLeft;
    Ray upRight;
    Ray downRight;
    Ray downLeft;

    jadel::Vec2 upLeftPos = position + jadel::Vec2(-actorRadius, actorRadius);
    jadel::Vec2 upRightPos = position + jadel::Vec2(actorRadius, actorRadius);
    jadel::Vec2 downRightPos = position + jadel::Vec2(actorRadius, -actorRadius);
    jadel::Vec2 downLeftPos = position + jadel::Vec2(-actorRadius, -actorRadius);

    jadel::Vec2 modifiedVelocity = velocity;
    Ray raysFired[3];
    int numRaysFired;
    int i = 0;
    do
    {
        numRaysFired = 0;
        jadel::Vec2 dir = modifiedVelocity.normalize();
        bool down = dir.y < 0;
        bool up = dir.y > 0;
        bool left = dir.x < 0;
        bool right = dir.x > 0;
        if (up || left)
        {
            upLeft.shoot(upLeftPos, dir, ceilf(velLength) + 1, map);
            raysFired[numRaysFired++] = upLeft;
        }
        if (up || right)
        {
            upRight.shoot(upRightPos, dir, ceilf(velLength) + 1, map);
            raysFired[numRaysFired++] = upRight;
        }
        if (down || right)
        {
            downRight.shoot(downRightPos, dir, ceilf(velLength) + 1, map);
            raysFired[numRaysFired++] = downRight;
        }
        if (down || left)
        {
            downLeft.shoot(downLeftPos, dir, ceilf(velLength) + 1, map);
            raysFired[numRaysFired++] = downLeft;
        }

        Ray *shortestCollidedRay = NULL;
        jadel::Vec2 shortestRayVector;
        for (int i = 0; i < numRaysFired; ++i)
        {
            Ray *currentRay = &raysFired[i];
            if (currentRay->rayEndContent->barrier)
            {
                jadel::Vec2 currentRayVector = raysFired[i].rayVector();
                if (!shortestCollidedRay || isVec2ALongerThanB(shortestRayVector, currentRayVector))
                {
                    shortestCollidedRay = currentRay;
                    shortestRayVector = currentRayVector;
                }
            }
        }
        if (!shortestCollidedRay)
        {
            break;
        }

        bool xCollision = (left || right) && (shortestCollidedRay->isClippedOnCorner || (!shortestCollidedRay->isVerticallyClipped && (fabs(modifiedVelocity.x) > fabs(shortestRayVector.x))));
        bool yCollision = shortestCollidedRay->isVerticallyClipped && (fabs(modifiedVelocity.y) > fabs(shortestRayVector.y));

        if (!xCollision && !yCollision)
        {
            break;
        }
        // jadel::Vec2 edgeDistFromWall(rayVector.x - movementDirX * actorRadius, rayVector.y - movementDirY * actorRadius);
        if (yCollision)
        {
            modifiedVelocity.y = shortestRayVector.y;
            this->yCollision = true;
        }
        if (xCollision)
        {
            modifiedVelocity.x = shortestRayVector.x;
            this->xCollision = true;
        }

        ++i;
        if (i == 5)
            break;
    } while (xCollision || yCollision);

    for (int i = 0; i < numRaysFired; ++i)
    {
        currentGameState.minimap.pushLine(raysFired[i].start, raysFired[i].end, {1, 1, 1, 0});
    }

    this->checkedVelocity = modifiedVelocity;
    bool result = xCollision || yCollision;
    return result;
}