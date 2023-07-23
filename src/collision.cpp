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
    
    if (velocity.x == 0 && velocity.y == 0)
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

    static float edgeMargin = 0.01f;
    do
    {
        float velLength = modifiedVelocity.length();
        numRaysFired = 0;

        bool down = modifiedVelocity.y < 0;
        bool up = modifiedVelocity.y > 0;
        bool left = modifiedVelocity.x < 0;
        bool right = modifiedVelocity.x > 0;
        if (up || left)
        {
            upLeft.shoot(upLeftPos, modifiedVelocity, velLength + edgeMargin, map);
            raysFired[numRaysFired++] = upLeft;
        }
        if (up || right)
        {
            upRight.shoot(upRightPos, modifiedVelocity, velLength + edgeMargin, map);
            raysFired[numRaysFired++] = upRight;
        }
        if (down || right)
        {
            downRight.shoot(downRightPos, modifiedVelocity, velLength + edgeMargin, map);
            raysFired[numRaysFired++] = downRight;
        }
        if (down || left)
        {
            downLeft.shoot(downLeftPos, modifiedVelocity, velLength + edgeMargin, map);
            raysFired[numRaysFired++] = downLeft;
        }

        Ray *shortestCollidedRay = NULL;
        jadel::Vec2 shortestRayVector;
        for (int i = 0; i < numRaysFired; ++i)
        {
            Ray *currentRay = &raysFired[i];
            RayClip rayClip = currentRay->getRayEndClip();
            if (rayClip.content.type == RAY_CONTENT_TYPE_SECTOR && rayClip.content.barrier)
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
        RayClip shortestRayClip = shortestCollidedRay->getRayEndClip();
    
        bool xCollision = (left || right) && (shortestRayClip.content.isClippedOnCorner || (!shortestRayClip.content.isVerticallyClipped && (fabs(modifiedVelocity.x) > fabs(shortestRayVector.x) - edgeMargin * 2)));
        bool yCollision = (up || down) && (shortestRayClip.content.isVerticallyClipped && (fabs(modifiedVelocity.y) > fabs(shortestRayVector.y) - edgeMargin));
        //bool xCollision = (fabs(modifiedVelocity.x) > fabs(shortestRayVector.x)) - 0.01f;
        //bool yCollision = (fabs(modifiedVelocity.y) > fabs(shortestRayVector.y)) - 0.01f;

        if (!xCollision && !yCollision)
        {
            break;
        }
        // jadel::Vec2 edgeDistFromWall(rayVector.x - movementDirX * actorRadius, rayVector.y - movementDirY * actorRadius);

        if (yCollision)
        {
            modifiedVelocity.y = shortestRayVector.y - (modifiedVelocity.y > 0 ? 1.0f : -1.0f) * edgeMargin;
            this->yCollision = true;
            
        }
        else if (xCollision)
        {
            modifiedVelocity.x = shortestRayVector.x - (modifiedVelocity.x > 0 ? 1.0f : -1.0f) * edgeMargin;
            this->xCollision = true;
        }

        if (shortestRayClip.content.isClippedOnCorner)
        {
            jadel::message("Corner collision!\n");
        }

        ++i;
        if (i == 5)
            break;
    } while (xCollision || yCollision);

    for (int i = 0; i < numRaysFired; ++i)
    {
        currentGameState.minimap.pushLine(raysFired[i].start, raysFired[i].start + raysFired[i].rayVector().normalize() * 0.5f, {1, 1, 1, 0});
    }

    this->checkedVelocity = modifiedVelocity;
    bool result = xCollision || yCollision;
    return result;
}