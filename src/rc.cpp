#include <jadel.h>
#include <thread>
#include "minimap.h"
#include "rc.h"
#include "render.h"
#include "ray.h"
#include "saveload.h"
#include "util.h"
#include "debug.h"
#include "timer.h"
#include "defs.h"

static int myArgc;
static char **myArgv;

int screenWidth = 1280;
int screenHeight = 720;

float aspectWidth = (float)screenWidth / (float)screenHeight;
float aspectHeight = (float)screenHeight / (float)screenWidth;

float frameTime;

static float standardMovementSpeed = 1.2f;
static float runningMovementSpeed = 2.4f;

static int currentRayMaxDistance = 10;
static Timer frameTimer;

GameState currentGameState;

Actor *actors;
size_t numActors;
size_t maxActors;

jadel::Vec2 camPos(1, 1);
jadel::Vec2 lastCamPos(1, 1);

static Minimap minimap;

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

        Ray rayResult;
        rayResult.shoot(rayStartPos, camToScreenColumnVector.normalize(), currentRayMaxDistance, &currentGameState.map);

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

        if (rayResult.rayEndContent->barrier)
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
                minimap.pushRect({(float)x, (float)y, x + 1.0f, y + 1.0f}, sectorColor);
                // jadel::graphicsDrawRectRelative(dim, sectorColor);
                //            pushRectRenderable(dim, sectorColor);
            }
        }
        jadel::Vec2 mapDim((float)map->width * 0.05f, (float)map->height * 0.05f);
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
    Player *player = &currentGameState.player;
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
        Map *map = &currentGameState.map;
        Ray movementRay;
        Ray movementRayLeft;
        Ray movementRayRight;
        movementRay.shoot(player->actor.pos + dir * playerRadius, frameCorrectVel, ceilf(frameCorrectVel.length()) + 2.0f, map);
        movementRayLeft.shoot(playerLeftPoint, frameCorrectVel, ceilf(frameCorrectVel.length()), map);
        movementRayRight.shoot(playerRightPoint, frameCorrectVel, ceilf(frameCorrectVel.length()), map);

        Ray rayResults[3] = {movementRay, movementRayLeft, movementRayRight};
        float movementDirX = player->actor.vel.x > 0 ? 1.0f : -1.0f;
        float movementDirY = player->actor.vel.y > 0 ? 1.0f : -1.0f;

        jadel::Color collisionColor = {1, 1, 0, 0};
        jadel::Color noCollisionColor = {1, 0, 1, 0};
        jadel::Color movementLineColors[3] = {noCollisionColor, noCollisionColor, noCollisionColor};
        int numVerticalClips = 0;

        for (int i = 0; i < 3; ++i)
        {
            Ray *ray = &rayResults[i];
            if (ray->isVerticallyClipped)
                ++numVerticalClips;
        }

        int shortestRayIndex = -1;

        for (int i = 0; i < 3; ++i)
        {
            Ray *ray = &rayResults[i];
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
            if (ray->rayEndContent->barrier)
            {
                if (shortestRayIndex == -1 || rayResults[shortestRayIndex].length() > ray->length())
                {
                    shortestRayIndex = i;
                }
            }
        }

        if (shortestRayIndex != -1)
        {
            Ray *shortestRay = &rayResults[shortestRayIndex];
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

void GameState::setMap(Map *map)
{
    if (map->isSavedToFile)
        jadel::message("\nMap file name: %s\n", map->fileName);
    jadel::message("Map name: %s\n", map->name);
    for (int y = 0; y < map->height; ++y)
    {
        for (int x = 0; x < map->width; ++x)
        {
            jadel::message("%d", map->getSectorContent(x, y)->barrier);
        }
        jadel::message("\n");
    }
    this->map = *map;
}

void tick()
{
    Player *player = &currentGameState.player;
    player->actor.vel *= 0;
    // jadel::message("sc W: %f, sc D: %f\n", screenPlaneWidth, screenPlaneDist);
    minimap.clear();

    if (jadel::inputIsKeyTyped(jadel::KEY_Q))
    {
        saveGameToFile("save/save01", &currentGameState);
        // jadel::memoryPrintDebugData();
        //currentGameState.map.saveToFile("maps/map02");
    }

    if (jadel::inputIsKeyTyped(jadel::KEY_E))
    {
        loadSaveFromFile("save/save01", &currentGameState);
/*
        Map map;
        Map::loadFromFile("maps/map02.rcmap", &map);
        currentGameState.setMap(&map);*/
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
        currentGameState.showMiniMap = !currentGameState.showMiniMap;
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

bool setGameState(const GameState *state)
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
    bool mapLoaded = false;
    maxActors = 100;
    numActors = 0;
    Map map;
    actors = (Actor *)jadel::memoryReserve(maxActors * sizeof(Actor));
    pushActor(2.5f, 3.5f, {0, 0, 0.3f, 0.7f});
    pushActor(5.5f, 5.5f, {0, 0, 0.4f, 0.6f});

    if (myArgc > 1)
    {
        for (int i = 1; i < myArgc; ++i)
        {
            if (isStringHeader(".sv", myArgv[i]))
            {
                gameStateSet = loadSaveFromFile(myArgv[i], &currentGameState);
            }
            else if (isStringHeader(".rcmap", myArgv[i]))
            {
                mapLoaded = Map::loadFromFile(myArgv[i], &map);
            }
        }
    }
    if (!gameStateSet)
    {
        currentGameState.player.actor.pos = jadel::Vec2(1.5f, 1.5f);
        currentGameState.player.actor.facingAngle = 0;
        currentGameState.showMiniMap = false;
    }
    // currentGameState.map.init(20, 15);
    if (!mapLoaded)
    {
        Map::loadFromFile("maps/map01.rcmap", &map);
    }
    currentGameState.setMap(&map);

    currentGameState.player.actor.movementSpeed = standardMovementSpeed;
    currentGameState.player.actor.turningSpeed = 140.0f;
    camPos = currentGameState.player.actor.pos;
    camRotation = currentGameState.player.actor.facingAngle;

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

    /*Map map;
    map.init(10, 10, "Second map");

    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            int index = x + y * map.width;
            map.sectors[index].barrier = x % 3 == 0 && y % 3 == 0;
            map.sectors[index].color =
            {1, 1, 0.1f, 0.1f};
        }
    }
    map.saveToFile("maps/map02");*/
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