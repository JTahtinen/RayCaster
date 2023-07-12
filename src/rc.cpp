#include <jadel.h>
#include <thread>
#include "debug.h"
#include "timer.h"

#define MATH_PI (3.141592653)
#define TO_RADIANS(deg) (deg * MATH_PI / 180.0)
#define TO_DEGREES(rad) (rad * (180.0 / MATH_PI))

int myArgc;
char **myArgv;

static int screenWidth = 1280;
static int screenHeight = 720;

static float aspectWidth = (float)screenWidth / (float)screenHeight;
static float aspectHeight = (float)screenHeight / (float)screenWidth;

static float frameTime;

#define MAP_WIDTH (20)
#define MAP_HEIGHT (15)

static int map[MAP_WIDTH * MAP_HEIGHT];

static float standardMovementSpeed = 1.2f;
static float runningMovementSpeed = 2.4f;

static bool showMiniMap = false;

static int currentRayMaxDistance = 10;
static Timer frameTimer;
struct Actor
{
    jadel::Vec2 pos;
    jadel::Vec2 vel;
    jadel::Rectf dim;
    float facingAngle;
    float movementSpeed;
    float turningSpeed;
};

struct Player
{
    Actor actor;
};

struct GameState
{
    Player player;
};

static GameState currentGameState;

Actor *actors;
size_t numActors;
size_t maxActors;

struct Line
{
    jadel::Vec2 start;
    jadel::Vec2 end;
    jadel::Color color;
};

struct RectRenderable
{
    jadel::Rectf rect;
    jadel::Color color;
};

struct RayResult
{
    jadel::Vec2 start;
    jadel::Vec2 end;
    bool isVerticallyClipped;
    bool isClippedOnCorner;
    int rayEndContent;

    inline float length() const
    {
        float result = rayVector().length();
        return result;
    }

    inline jadel::Vec2 rayVector() const
    {
        jadel::Vec2 result = end - start;
        return result;
    }
};

struct Minimap
{
    jadel::Vec2 screenStart;
    float scale;
    RectRenderable *rects;
    size_t maxRects;
    size_t numRects;
    Line *lines;
    size_t maxLines;
    size_t numLines;

    int pushRect(jadel::Rectf rect, jadel::Color color)
    {
        if (numRects >= maxRects)
            return 0;
        rects[numRects++] = {rect, color};
        return 1;
    }

    int pushRect(jadel::Vec2 p0, jadel::Vec2 p1, jadel::Color color)
    {
        return pushRect(jadel::Rectf(p0, p1), color);
    }

    int pushLine(jadel::Vec2 start, jadel::Vec2 end, jadel::Color color)
    {
        if (numLines >= maxLines)
            return 0;
        this->lines[this->numLines++] = {start, end, color};
        return 1;
    }

    jadel::Vec2 findPointOnScreen(jadel::Vec2 point) const
    {
        jadel::Vec2 result = this->screenStart + (jadel::Vec2(point.x * aspectHeight, point.y) * this->scale);
        return result;
    }

    void clear()
    {
        this->numRects = 0;
        this->numLines = 0;
    }
};

jadel::Vec2 camPos(1, 1);
jadel::Vec2 lastCamPos(1, 1);

static Minimap minimap;

jadel::Vec2 clipSegmentFromLineAtY(jadel::Vec2 source, float y)
{
    if (source.y == 0)
        return source;
    float ratio = source.x / source.y;
    jadel::Vec2 result(ratio * y, y);
    return result;
}

jadel::Vec2 clipSegmentFromLineAtX(jadel::Vec2 source, float x)
{
    if (source.x == 0)
        return source;
    float ratio = source.y / source.x;
    jadel::Vec2 result(x, ratio * x);
    return result;
}

int getSectorContent(int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return 0;
    return map[x + y * MAP_WIDTH];
}

bool isVec2ALongerThanB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y) > (b.x * b.x + b.y * b.y);
    return result;
}

bool isVec2ALengthEqualToB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y) == (b.x * b.x + b.y * b.y);
    return result;
}

float getCosBetweenVectors(jadel::Vec2 a, jadel::Vec2 b)
{
    float result = a.dot(b) / (a.length() + b.length());
    return result;
}

bool isValueBetween(float min, float max, float val)
{
    bool result = min < val && max >= val;
    return result;
}

float moduloFloat(float value, int mod)
{
    float result = value;
    if (!isValueBetween(0, (float)mod, value))
    {
        float remainder = value - (int)value;
        float fixedValue = (int)(roundf(value)) % mod + remainder;
        if (fixedValue < 0)
            fixedValue += (float)mod;
        result = fixedValue;
    }
    return result;
}

jadel::Mat3 getRotationMatrix(float angleInDegrees)
{
    float finalAngle = moduloFloat(angleInDegrees, 360);
    float radAngle = TO_RADIANS(finalAngle);
    jadel::Mat3 result =
        {
            cosf(radAngle), sinf(radAngle), 0,
            -sinf(radAngle), cosf(radAngle), 0,
            0, 0, 1};
    return result;
}

void setActorRotation(Actor *actor, float angle)
{
    if (!actor)
        return;
    actor->facingAngle = moduloFloat(angle, 360);
}

void rotateActorTowards(Actor *actor, int dir)
{
    if (!actor)
        return;
    setActorRotation(actor, actor->facingAngle - (float)dir * actor->turningSpeed * frameTime);
}

static float camRotation = 0;

jadel::Mat3 cameraRotationMatrix;
static float screenPlaneDist = 0.15f;
static float screenPlaneWidth = screenPlaneDist / aspectHeight;

int shootRay(const jadel::Vec2 start, const jadel::Vec2 dir, uint32 maxDistance, RayResult *result)
{
    if (maxDistance < 1)
        return 0;
    int isXDirPositive = dir.x > 0 ? 1 : 0;
    int isYDirPositive = dir.y > 0 ? 1 : 0;

    // jadel::Vec2 unRotatedScreenClip = clipSegmentFromLineAtY(unRotatedRayLine, defaultScreenPlane.y);
    // jadel::Vec2 rotatedScreenClip = cameraRotationMatrix.mul(unRotatedScreenClip);

    int rayEndContent = 0;

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
        if (rayMarchPos.x <= 0 || rayMarchPos.y <= 0 || rayMarchPos.x >= MAP_WIDTH || rayMarchPos.y >= MAP_HEIGHT)
            break;

        bool xMarginNeeded = ((!isXDirPositive && !raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);
        bool yMarginNeeded = ((!isYDirPositive && raySegmentIsVerticallyClipped) || raySegmentIsClippedOnCorner);

        float xMargin = xMarginNeeded * (-0.1f);
        float yMargin = yMarginNeeded * (-0.1f);

        rayEndContent = getSectorContent(floorf(rayMarchPos.x + xMargin), floorf(rayMarchPos.y + yMargin));
        if (rayEndContent)
        {
            break;
        }
    }
    result->start = start;
    result->end = rayMarchPos;
    result->isVerticallyClipped = raySegmentIsVerticallyClipped;
    result->isClippedOnCorner = raySegmentIsClippedOnCorner;
    result->rayEndContent = rayEndContent;
    return 1;
}

float getAngleOfVec2(jadel::Vec2 vec)
{
    if (vec.x < 0)
    {
        return 360.0f + TO_DEGREES(atan2(vec.x, vec.y));
    }
    else
    {
        return TO_DEGREES(atan2(vec.x, vec.y));
    }
}

bool collision = false;
static float internalHorizontalResolution = screenWidth;
static int resolutionDivider = 1;
void render()
{
    jadel::graphicsClearTargetSurface();

    jadel::graphicsDrawRectRelative({-1, -1, 1, 0}, {1, 0.3f, 0.4f, 0.25f});
    jadel::graphicsDrawRectRelative({-1, 0, 1, 1}, {1, 0.2f, 0.2f, 0.4f});
    static jadel::Vec2 camPosMapDim(0.3f, 0.3f);

    minimap.pushRect(jadel::Rectf(camPos - camPosMapDim, camPos + camPosMapDim), {1, 1, 0, 0});

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

    for (int seg = 0; seg < internalHorizontalResolution; ++seg)
    {
        jadel::Vec2 unRotatedCamToScreenRay(unRotatedVirtualScreenStartClip.x + (float)seg * (screenPlaneWidth / (float)internalHorizontalResolution), screenPlaneDist); // Can be modified, so x should not be set to currentScreenX
        // unRotatedRayLine = clipSegmentFromLineAtY(unRotatedRayLine, 1.0f);
        jadel::Vec2 camToScreenColumnVector = cameraRotationMatrix.mul(unRotatedCamToScreenRay);
        jadel::Vec2 rayStartPos = camPos + camToScreenColumnVector;

        RayResult rayResult;
        shootRay(rayStartPos, camToScreenColumnVector.normalize(), currentRayMaxDistance, &rayResult);

        if (seg % 200 == 0 || seg == internalHorizontalResolution - 1)
        {
            minimap.pushLine(rayResult.start, rayResult.end, {1, 0, 1, 0});
            minimap.pushLine(camPos, rayResult.start, {1, 1, 0, 1});

            static jadel::Vec2 clipPointDim(0.1f, 0.1f);
            minimap.pushRect(rayResult.end - clipPointDim, rayResult.end + clipPointDim, {1, 1, 1, 0});
            if (seg == 0 || seg == internalHorizontalResolution - 1)
            {
                minimap.pushLine(camPos, rayResult.end, {1, 1, 0, 0});
            }
        }

        if (rayResult.rayEndContent == 1)
        {
            float dist = rayResult.length();
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

                jadel::Color defaultWallColor = {1, defaultWallScale, defaultWallScale, defaultWallScale};
                float scale = jadel::lerp(obliqueWallScale, defaultWallScale, 1.0f - cosWallAngle);
                static float ambientLight = 0;
                float distanceSquared = (dist * dist) * 0.3;
                jadel::Color wallColor = {defaultWallColor.a,
                                          jadel::clampf(scale / distanceSquared, 0, scale) + ambientLight + 0.2f,
                                          jadel::clampf(scale / distanceSquared, 0, scale) + ambientLight,
                                          jadel::clampf(scale / distanceSquared, 0, scale) + ambientLight + 0.2f};
                float currentScreenX = -edgeXDiff + (float)seg * columnWidth;
                jadel::graphicsDrawRectRelative({currentScreenX, -1.0f / (dist), currentScreenX + columnWidth, 1.0f / (dist)}, wallColor);
            }
        }
    }
    minimap.pushLine(virtualScreenWorldStart, virtualScreenWorldEnd, {1, 0, 0, 1});

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

    if (showMiniMap)
    {
        for (int y = 0; y < MAP_HEIGHT; ++y)
        {
            for (int x = 0; x < MAP_WIDTH; ++x)
            {
                int content = getSectorContent(x, y);
                static jadel::Color occupiedSectorColor = {0.7f, 0.3f, 0.3f, 0.3f};
                static jadel::Color emptySectorColor = {0.7f, 0.2f, 0.2f, 0.2f};
                jadel::Color sectorColor = content == 0 ? emptySectorColor : occupiedSectorColor;
                minimap.pushRect({(float)x, (float)y, x + 1.0f, y + 1.0f}, sectorColor);
                // jadel::graphicsDrawRectRelative(dim, sectorColor);
                //            pushRectRenderable(dim, sectorColor);
            }
        }
        jadel::Vec2 mapDim((float)MAP_WIDTH * 0.05f, (float)MAP_HEIGHT * 0.05f);
        jadel::Vec2 camMapPos = minimapStart + jadel::Vec2(camPos.x * 0.05f, camPos.y * 0.05f);
        //  pushRectRenderable({camMapPos.x - 0.005f, camMapPos.y - 0.005f, camMapPos.x + 0.005f, camMapPos.y + 0.005f}, {0.7f, 1, 0, 0});
        // jadel::message("Minimap rects: %d, lines: %d\n", minimap.numRects, minimap.numLines);
        for (int i = 0; i < minimap.numRects; ++i)
        {
            RectRenderable renderable = minimap.rects[i];
            jadel::Rectf rect = renderable.rect;
            jadel::Rectf perspectiveRect(
                minimap.findPointOnScreen(rect.getPoint0()),
                minimap.findPointOnScreen(rect.getPoint1()));
            jadel::graphicsDrawRectRelative(perspectiveRect, renderable.color);
        }

        for (int i = 0; i < minimap.numLines; ++i)
        {
            Line renderable = minimap.lines[i];
            jadel::Vec2 start = renderable.start;
            jadel::Vec2 end = renderable.end;
            jadel::Vec2 perspectiveStart = minimap.findPointOnScreen(start);
            jadel::Vec2 perspectiveEnd = minimap.findPointOnScreen(end);

            jadel::graphicsDrawLineRelative(perspectiveStart, perspectiveEnd, renderable.color);
        }
    }
    if (collision)
        jadel::graphicsDrawRectRelative(-0.8f, -0.8f, -0.7f, -0.7f, {1, 1, 0, 0});
}

jadel::Vec2 frameCorrectVel;

void collisionCheck()
{
    Player* player = &currentGameState.player;
    frameCorrectVel = player->actor.vel * frameTime;
    float playerRadius = 0.3f;

    jadel::Vec2 dir = player->actor.vel.normalize();
    float dirAngle = 360.0f - getAngleOfVec2(dir);
    jadel::Mat3 playerRotation = getRotationMatrix(player->actor.facingAngle);
    jadel::Mat3 movementDirRotation = getRotationMatrix(dirAngle);

    jadel::Vec2 playerLeftPoint;
    jadel::Vec2 playerRightPoint;

    if (dir.length() > 0)
    {
        playerLeftPoint = player->actor.pos + movementDirRotation.mul(jadel::Vec2(-0.3f, 0.3f));
        playerRightPoint = player->actor.pos + movementDirRotation.mul(jadel::Vec2(0.3f, 0.3f));
    }
    else
    {
        playerLeftPoint = player->actor.pos + playerRotation.mul(jadel::Vec2(-0.3f, 0));
        playerRightPoint = player->actor.pos + playerRotation.mul(jadel::Vec2(0.3f, 0));
    }
    collision = false;
    if (player->actor.vel.length() > 0)
    {
        RayResult movementRay;
        RayResult movementRayLeft;
        RayResult movementRayRight;
        shootRay(player->actor.pos + dir * playerRadius, frameCorrectVel, ceilf(frameCorrectVel.length()) + 2.0f, &movementRay);
        shootRay(playerLeftPoint, frameCorrectVel, ceilf(frameCorrectVel.length()), &movementRayLeft);
        shootRay(playerRightPoint, frameCorrectVel, ceilf(frameCorrectVel.length()), &movementRayRight);

        RayResult rayResults[3] = {movementRay, movementRayLeft, movementRayRight};
        float movementDirX = player->actor.vel.x > 0 ? 1.0f : -1.0f;
        float movementDirY = player->actor.vel.y > 0 ? 1.0f : -1.0f;

        jadel::Color collisionColor = {1, 1, 0, 0};
        jadel::Color noCollisionColor = {1, 0, 1, 0};
        jadel::Color movementLineColors[3] = {noCollisionColor, noCollisionColor, noCollisionColor};
        int numVerticalClips = 0;

        for (int i = 0; i < 3; ++i)
        {
            RayResult *ray = &rayResults[i];
            if (ray->isVerticallyClipped)
                ++numVerticalClips;
        }

        int shortestRayIndex = -1;

        for (int i = 0; i < 3; ++i)
        {
            RayResult *ray = &rayResults[i];
            if (numVerticalClips >= 2)
            {
                if (!ray->isVerticallyClipped)
                    continue;
            }
            else
            {
                if (ray->isVerticallyClipped)
                    continue;
            }
            if (ray->rayEndContent == 1)
            {
                if (shortestRayIndex == -1 || rayResults[shortestRayIndex].length() > ray->length())
                {
                    shortestRayIndex = i;
                }
            }
        }

        if (shortestRayIndex != -1)
        {
            RayResult *shortestRay = &rayResults[shortestRayIndex];
            jadel::Vec2 rayVector = shortestRay->rayVector();
            if (shortestRay->isVerticallyClipped)
            {
                if (fabs(rayVector.y) <= fabs(frameCorrectVel.y - 0.001f))
                {
                    frameCorrectVel.y = rayVector.y - movementDirY * 0.001;
                    movementLineColors[shortestRayIndex] = collisionColor;
                }
            }
            else
            {
                if (fabs(rayVector.x) <= fabs(frameCorrectVel.x - 0.001))
                {
                    frameCorrectVel.x = rayVector.x - movementDirX * 0.001;
                    movementLineColors[shortestRayIndex] = collisionColor;
                }
            }
        }
    }
}

bool loadSave(const char *filepath, GameState *target)
{
    jadel::BinaryFile file;
    if (!target)
    {
        jadel::message("[ERROR] Could not load save file %s. No target file!\n", filepath);
        return false;
    }
    if (!file.init(filepath))
    {
        jadel::message("[ERROR] Could not load save file %s. File doesn't exist!\n", filepath);
        return false;
    }
    if (file.fileBufferSize != 3 * sizeof(float))
    {
        jadel::message("[ERROR] Could not load save file %s. Corrupt file!\n", filepath);
        return false;
    }
    file.readFloat(&target->player.actor.pos.x);
    file.readFloat(&target->player.actor.pos.y);
    file.readFloat(&target->player.actor.facingAngle);
    jadel::message("Loaded game: %s\n", filepath);
    file.close();
    return true;
}

bool save(const char *filepath, const GameState* state)
{
    jadel::BinaryFile file;
    if (!file.init(3 * sizeof(float)))
    {
        jadel::message("[ERROR] Could not save game file: %\n", filepath);
        return false;
    }
    file.writeFloat(state->player.actor.pos.x);
    file.writeFloat(state->player.actor.pos.y);
    file.writeFloat(state->player.actor.facingAngle);
    file.writeToFile(filepath);
    jadel::message("Saved game: %s\n", filepath);
    file.close();
    return true;
}

void tick()
{
    Player* player = &currentGameState.player;
    player->actor.vel *= 0;
    // jadel::message("sc W: %f, sc D: %f\n", screenPlaneWidth, screenPlaneDist);
    minimap.clear();

    if (jadel::inputIsKeyTyped(jadel::KEY_Q))
    {
        save("save/save01.sv", &currentGameState);
    }

    if (jadel::inputIsKeyTyped(jadel::KEY_E))
    {
        loadSave("save/save01.sv", &currentGameState);
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_N))
    {
        if (resolutionDivider < screenWidth)
            resolutionDivider += 2;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_M))
    {
        if (resolutionDivider > 1)
            resolutionDivider -= 2;
    }
    if (jadel::inputIsKeyTyped(jadel::KEY_PAGEUP))
    {
        ++currentRayMaxDistance;
    }

    if (jadel::inputIsKeyTyped(jadel::KEY_PAGEDOWN))
    {
        --currentRayMaxDistance;
        if (currentRayMaxDistance < 1)
            currentRayMaxDistance = 1;
    }
    if (jadel::inputIsKeyTyped(jadel::KEY_TAB))
    {
        showMiniMap = !showMiniMap;
    }
    if (jadel::inputIsKeyTyped(jadel::KEY_SHIFT))
    {
        player->actor.movementSpeed = runningMovementSpeed;
    }
    else if (jadel::inputIsKeyReleased(jadel::KEY_SHIFT))
    {
        player->actor.movementSpeed = standardMovementSpeed;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_X))
    {
        screenPlaneDist += 0.8f * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_Z))
    {
        screenPlaneDist -= 0.8f * frameTime;
    }

    if (jadel::inputIsKeyPressed(jadel::KEY_V))
    {
        screenPlaneWidth += (0.8f * screenPlaneWidth) * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_C))
    {
        screenPlaneWidth -= (0.8f * screenPlaneWidth) * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_LEFT))
    {
        rotateActorTowards(&player->actor, -1);
        // player.actor.facingAngle += player.turningSpeed * frameTime;
    }

    if (jadel::inputIsKeyPressed(jadel::KEY_RIGHT))
    {
        rotateActorTowards(&player->actor, 1);
        // player->actor.facingAngle -= player->turningSpeed * frameTime;
    }

    camRotation = player->actor.facingAngle;

    cameraRotationMatrix = getRotationMatrix(camRotation);
    jadel::Mat3 rotate90DegMatrix = getRotationMatrix(90);

    jadel::Vec2 forward = cameraRotationMatrix.mul(jadel::Vec2(0, 1.0f));
    jadel::Vec2 left = rotate90DegMatrix.mul(forward);

    if (jadel::inputIsKeyPressed(jadel::KEY_A))
    {
        player->actor.vel += left * player->actor.movementSpeed;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_D))
    {
        player->actor.vel -= left * player->actor.movementSpeed;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_S))
    {
        player->actor.vel -= forward * player->actor.movementSpeed;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_W))
    {
        player->actor.vel += forward * player->actor.movementSpeed;
    }

    collisionCheck();
    player->actor.pos += frameCorrectVel;

    camPos = player->actor.pos;

    lastCamPos = camPos;

    render();
}

bool createDefaultActor(float x, float y, Actor *target)
{
    if (!target)
        return false;
    target->dim = {0, 0, 0.3f, 0.7f};
    target->pos = jadel::Vec2(x, y);
    target->facingAngle = 90.0f;
    target->turningSpeed = 140.0f;
    target->movementSpeed = 1.2f;
    return true;
}

void pushActor(float x, float y, jadel::Rectf dim)
{
    if (numActors >= maxActors)
        return;
    Actor *actor = &actors[numActors++];
    createDefaultActor(x, y, actor);
    actor->dim = dim;
}

GameState gameStartState = {{jadel::Vec2(1.5f, 1.5f)}};

bool setGameState(const GameState* state)
{
    if (!state)
    {
        jadel::message("[ERROR] NULL Game state!\n");
        return false;
    }
    currentGameState = *state;
    return true;
}

void init()
{

    bool gameStateSet = false;
    maxActors = 100;
    numActors = 0;
    actors = (Actor *)jadel::memoryReserve(maxActors * sizeof(Actor));
    pushActor(2.5f, 3.5f, {0, 0, 0.3f, 0.7f});
    pushActor(5.5f, 5.5f, {0, 0, 0.4f, 0.6f});


    if (myArgc > 1)
    {
        gameStateSet = loadSave(myArgv[1], &currentGameState);
    }
    if (!gameStateSet)
    {
        currentGameState = gameStartState;
    }
    currentGameState.player.actor.movementSpeed = standardMovementSpeed;
    currentGameState.player.actor.turningSpeed = 140.0f;
    camPos = currentGameState.player.actor.pos;
    camRotation = currentGameState.player.actor.facingAngle;
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            int index = x + y * MAP_WIDTH;
            map[index] = y % 2 == 0 && x % 2 == 0 ? 1 : 0;
            //jadel::message("%d ", map[index]);
        }
        //jadel::message("\n");
    }

    minimap.maxRects = 500;
    minimap.maxLines = 500;
    minimap.rects = (RectRenderable *)jadel::memoryReserve(minimap.maxRects * sizeof(RectRenderable));
    minimap.lines = (Line *)jadel::memoryReserve(minimap.maxLines * sizeof(Line));
    minimap.scale = 0.05f;
    minimap.screenStart = jadel::Vec2(0.2f, 0);
    minimap.clear();
}

int JadelMain(int argc, char **argv)
{
    myArgc = argc;
    myArgv = argv;
    if (!JadelInit(MB(500)))
    {
        jadel::message("Jadel init failed!\n");
        return 0;
    }
    jadel::allocateConsole();
    srand(time(NULL));
    for (int i = 0; i < argc; ++i)
    {
        jadel::message("%d: %s\n", i, argv[i]);
    }
#ifdef DEBUG
    if (!DEBUGInit())
    {
        jadel::message("[ERROR] Debug Init failed\n");
        return 0;
    }
#endif
    jadel::Window window;
    jadel::windowCreate(&window, "RayCaster", screenWidth, screenHeight);
    jadel::Surface winSurface;
    jadel::graphicsCreateSurface(screenWidth, screenHeight, &winSurface);
    uint32 *winPixels = (uint32 *)winSurface.pixels;

    jadel::graphicsPushTargetSurface(&winSurface);
    jadel::graphicsSetClearColor(0);
    jadel::graphicsClearTargetSurface();

    init();
    frameTimer.start();
    uint32 elapsedInMillis = 0;
    uint32 minFrameTime = 1000 / 165;
    uint32 accumulator = 0;
    int framesPerSecond = 0;
    while (true)
    {
        JadelUpdate();

        tick();
        ++framesPerSecond;
        jadel::windowUpdate(&window, &winSurface);
        elapsedInMillis = frameTimer.getMillisSinceLastUpdate();
        accumulator += elapsedInMillis;

#ifdef DEBUG
        // DEBUGClear("frame");
        if (accumulator >= 1000)
        {
            // DEBUGPushFloat(frameTime, "frame");
            // DEBUGPushInt(framesPerSecond, "frame");
            // DEBUGPrint("frame");
            accumulator %= 1000;
            framesPerSecond = 0;
        }
#endif
        if (elapsedInMillis < minFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(minFrameTime - elapsedInMillis));
        }
        if (elapsedInMillis > 0)
        {
            frameTime = (float)frameTimer.getMillisSinceLastUpdate() * 0.001f;
        }

        uint32 debugTime = frameTimer.getMillisSinceLastUpdate();
        // jadel::message("%f\n", frameTime);

        frameTimer.update();
        if (jadel::inputIsKeyPressed(jadel::KEY_ESCAPE))
            return 0;
    }
    return 0;
}