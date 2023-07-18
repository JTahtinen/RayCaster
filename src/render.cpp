#include "render.h"
#include "rc.h"
#include "ray.h"
#include "util.h"
#include <math.h>

float internalHorizontalResolution = screenWidth;
int resolutionDivider = 1;

float screenPlaneDist = 0.15f;
float screenPlaneWidth = screenPlaneDist / aspectHeight;

void setScreenPlane(float distance, float width)
{
    screenPlaneDist = distance;
    screenPlaneWidth = width;
}

void modScreenPlane(float distance, float width)
{
    setScreenPlane(screenPlaneDist + distance, screenPlaneWidth + width);
}

void render()
{
    jadel::graphicsClearTargetSurface();
    Minimap *minimap = &currentGameState.minimap;
    jadel::Mat3 cameraRotationMatrix = getRotationMatrix(currentGameState.camera.positionData.rotation);
    jadel::graphicsDrawRectRelative({-1, -1, 1, 0}, {1, 0.3f, 0.4f, 0.25f});
    jadel::graphicsDrawRectRelative({-1, 0, 1, 1}, {1, 0.2f, 0.2f, 0.4f});
    static jadel::Vec2 camPosMapDim(0.3f, 0.3f);

    jadel::Vec2 camPos = currentGameState.camera.positionData.position;
    minimap->pushRect(jadel::Rectf(camPos - camPosMapDim, camPos + camPosMapDim), {1, 1, 0, 0});

    internalHorizontalResolution = screenWidth / resolutionDivider;
    const float edgeXDiff = 1.0f;
    float columnWidth = (2.0f * edgeXDiff) / (float)internalHorizontalResolution;
    static jadel::Vec2 minimapStart(0.2f, 0);
    jadel::Vec2 minimapCameraStart = minimapStart + camPos * 0.05f;
    jadel::Vec2 unRotatedVirtualScreenStartClip(-screenPlaneWidth * 0.5f, screenPlaneDist);
    jadel::Vec2 unRotatedVirtualScreenEndClip(screenPlaneWidth * 0.5f, screenPlaneDist);
    jadel::Vec2 virtualScreenWorldStart = camPos + cameraRotationMatrix.mul(unRotatedVirtualScreenStartClip);
    jadel::Vec2 virtualScreenWorldEnd = camPos + cameraRotationMatrix.mul(unRotatedVirtualScreenEndClip);
    float screenPlaneXEnd = edgeXDiff;
    jadel::Mat3 cameraToScreenRotationMatrix = getRotationMatrix(360.0f - currentGameState.camera.getRotation());
    for (int seg = 0; seg < internalHorizontalResolution; ++seg)
    {
        jadel::Vec2 unRotatedCamToScreenRay(unRotatedVirtualScreenStartClip.x + (float)seg * (screenPlaneWidth / (float)internalHorizontalResolution), screenPlaneDist); // Can be modified, so x should not be set to currentScreenX
        // unRotatedRayLine = clipSegmentFromLineAtY(unRotatedRayLine, 1.0f);
        jadel::Vec2 camToScreenColumnVector = cameraRotationMatrix.mul(unRotatedCamToScreenRay);
        jadel::Vec2 rayStartPos = camPos;// + camToScreenColumnVector;

        Ray rayResult;
        rayResult.shoot(camPos, camToScreenColumnVector, 100, &currentGameState.map);

        /*
                if (seg % 200 == 0 || seg == internalHorizontalResolution / 2 || seg == internalHorizontalResolution - 1)
                {
                    minimap->pushLine(rayResult.start, rayResult.end, {1, 0, 1, 0});
                    minimap->pushLine(camPos, rayResult.start, {1, 1, 0, 1});

                    static jadel::Vec2 clipPointDim(0.1f, 0.1f);
                    minimap->pushRect(rayResult.end - clipPointDim, rayResult.end + clipPointDim, {1, 1, 1, 0});
                    if (seg == 0 || seg == internalHorizontalResolution - 1)
                    {
                        minimap->pushLine(camPos, rayResult.end, {1, 1, 0, 0});
                    }
                }
        */

        if (rayResult.rayEndContent->barrier)
        {
            float dist = rayResult.length();
            float range;
            jadel::Vec2 rayVector = rayResult.rayVector();
            jadel::Vec2 rotatedRayVector = cameraToScreenRotationMatrix.mul(rayVector);

            range = rotatedRayVector.y;

            if (dist > 0)
            {
                static float defaultWallScale = 0.8f;
                static float obliqueWallScale = 0.1f;
                float cosWallAngle;

                if (rayResult.isVerticallyClipped)
                {

                    cosWallAngle = fabs(getCosBetweenVectors(rayResult.end - camPos, jadel::Vec2(1.0f, 0)));
                }
                else
                {
                    cosWallAngle = fabs(getCosBetweenVectors(rayResult.end - camPos, jadel::Vec2(0, 1.0f)));
                }
                jadel::Color worldWallColor = rayResult.rayEndContent->color;
                float wallAngleFactor = 1.0f - cosWallAngle;
                // jadel::Color defaultWallColor = {1, defaultWallScale, defaultWallScale, defaultWallScale};
                float scale = jadel::lerp(obliqueWallScale, defaultWallScale, wallAngleFactor);

                static float ambientLight = 0.4f;
                float distanceSquared = (dist * dist);
                if (distanceSquared == 0)
                {
                    distanceSquared = 0.1f;
                }
                float distanceModifier = 1.0f / ((distanceSquared)*5.0f);
                float ambientRed = ambientLight * worldWallColor.r;
                float ambientGreen = ambientLight * worldWallColor.g;
                float ambientBlue = ambientLight * worldWallColor.b;

                jadel::Color wallColor = {worldWallColor.a,
                                          jadel::clampf((scale * worldWallColor.r) * distanceModifier + (ambientRed * scale), 0, worldWallColor.r + ambientRed),
                                          jadel::clampf((scale * worldWallColor.g) * distanceModifier + (ambientGreen * scale), 0, worldWallColor.g + ambientGreen),
                                          jadel::clampf((scale * worldWallColor.b) * distanceModifier + (ambientBlue * scale), 0, worldWallColor.b + ambientBlue)};
                float currentScreenX = -edgeXDiff + (float)seg * columnWidth;
                jadel::graphicsDrawRectRelative({currentScreenX, -1.0f / range, currentScreenX + columnWidth, 1.0f / range}, wallColor);
            }
        }
    }
    minimap->pushLine(virtualScreenWorldStart, virtualScreenWorldEnd, {1, 0, 0, 1});

    /*
        jadel::Mat3 viewRotationMatrix = getRotationMatrix(-camRotation);
        float screenToScreenPlaneRatio = 1.0f / (screenPlaneWidth / 2.0f);
        for (int a = 0; a < numActors; ++a)
        {
            Actor *actor = &actors[a];
            jadel::Vec2 translatedPosition = actor->pos - camPos;
            jadel::Vec2 finalPosition = viewRotationMatrix.mul(translatedPosition);
            finalPosition;

            float dist = finalPosition.length();
            float halfActorWidthOnScreen = (actor->dim.x1 * 0.5f) / dist;
            float actorHeightOnScreen = (actor->dim.y1) / dist;
            jadel::Rectf finalRect = {(finalPosition.x - halfActorWidthOnScreen) , -1.0f / dist, finalPosition.x + halfActorWidthOnScreen, -1.0f  / dist + actorHeightOnScreen};
            jadel::graphicsDrawRectRelative(finalRect, {1, 0.7, 0, 0});
            jadel::Vec2 actorMapPos = mapStart + jadel::Vec2(actor->pos.x * 0.05f, actor->pos.y * 0.05f);
            pushRectRenderable({actorMapPos.x - actor->dim.x1 * 0.05f, actorMapPos.y - actor->dim.x1 * 0.05f, actorMapPos.x + actor->dim.x1 * 0.05f, actorMapPos.y + actor->dim.x1 * 0.05f}, {0.7f, 0, 0, 1});
        }
        */

    if (currentGameState.showMiniMap)
    {
        Map *map = &currentGameState.map;
        for (int y = 0; y < map->height; ++y)
        {
            for (int x = 0; x < map->width; ++x)
            {
                bool barrier = map->getSectorContent(x, y)->barrier;
                static jadel::Color occupiedSectorColor = {0.7f, 0.3f, 0.3f, 0.3f};
                static jadel::Color emptySectorColor = {0.7f, 0.2f, 0.2f, 0.2f};
                jadel::Color sectorColor = !barrier ? emptySectorColor : occupiedSectorColor;
                minimap->pushRect({(float)x, (float)y, x + 1.0f, y + 1.0f}, sectorColor);
                // jadel::graphicsDrawRectRelative(dim, sectorColor);
                //            pushRectRenderable(dim, sectorColor);
            }
        }
        jadel::Vec2 mapDim((float)map->width * 0.05f, (float)map->height * 0.05f);
        jadel::Vec2 camMapPos = minimapStart + jadel::Vec2(camPos.x * 0.05f, camPos.y * 0.05f);
        //  pushRectRenderable({camMapPos.x - 0.005f, camMapPos.y - 0.005f, camMapPos.x + 0.005f, camMapPos.y + 0.005f}, {0.7f, 1, 0, 0});
        // jadel::message("Minimap rects: %d, lines: %d\n", minimap->numRects, minimap->numLines);
        for (int i = 0; i < minimap->numRects; ++i)
        {
            RectRenderable renderable = minimap->rects[i];
            jadel::Rectf rect = renderable.rect;
            jadel::Rectf perspectiveRect(
                minimap->findPointOnScreen(rect.getPoint0()),
                minimap->findPointOnScreen(rect.getPoint1()));
            jadel::graphicsDrawRectRelative(perspectiveRect, renderable.color);
        }

        for (int i = 0; i < minimap->numLines; ++i)
        {
            Line renderable = minimap->lines[i];
            jadel::Vec2 start = renderable.start;
            jadel::Vec2 end = renderable.end;
            jadel::Vec2 perspectiveStart = minimap->findPointOnScreen(start);
            jadel::Vec2 perspectiveEnd = minimap->findPointOnScreen(end);

            jadel::graphicsDrawLineRelative(perspectiveStart, perspectiveEnd, renderable.color);
        }
    }
}