#include <jadel.h>
#include <thread>
#include "imageload.h"
#include "minimap.h"
#include "rc.h"
#include "render.h"
#include "ray.h"
#include "saveload.h"
#include "util.h"
#include "debug.h"
#include "timer.h"
#include "defs.h"
#include "command.h"

AssetManager assetManager;

static int myArgc;
static char **myArgv;

bool relativeMouseMode = true;

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

jadel::Color createColorFrom(uint32 col)
{
    uint8 a = col >> 24;
    uint8 r = col >> 16;
    uint8 g = col >> 8;
    uint8 b = col;

    float aF = (float)a / 255.0f;
    float rF = (float)r / 255.0f;
    float gF = (float)g / 255.0f;
    float bF = (float)b / 255.0f;

    jadel::Color result = {aF, rF, gF, bF};
    return result;
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
            jadel::message("%d", map->getSector(x, y)->barrier);
        }
        jadel::message("\n");
    }
    this->map = *map;
}

void setMouseMode(bool mouseMode)
{
    relativeMouseMode = mouseMode;
    jadel::inputSetCursorVisible(!mouseMode);
    jadel::inputSetRelativeMouseMode(mouseMode);
}

void toggleMouseMode()
{
    setMouseMode(!relativeMouseMode);
}
float mouseSensitivity = 42.6f;
bool startEffectDone = false;
float effectStartDivision = 100;
Timer startEffectTimer;
float effectDurationInMillis = 2000;

void tick()
{
    if (!startEffectDone)
    {
        uint32 elapsed = startEffectTimer.getElapsedInMillis();

        resolutionDivider = (effectStartDivision + 1) - ((float)elapsed / (float)effectDurationInMillis) * effectStartDivision;

        if (elapsed >= effectDurationInMillis)
        {
            startEffectDone = true;
        }
        if (resolutionDivider < 1)
            resolutionDivider = 1;
        // jadel::message("%d\n", resolutionDivider);
    }
    static Texture *wallTex = assetManager.findTexture("textures/wall.png");
    if (!wallTex)
        exit(0);
    Player *player = &currentGameState.player;
    player->actor.vel *= 0;
    // jadel::message("sc W: %f, sc D: %f\n", screenPlaneWidth, screenPlaneDist);
    currentGameState.minimap.clear();

    CommandList commands;
    commands.update(&currentGameState.controls);
    uint32 nextCommand;
    jadel::Vec2 relativeDirVector(0, 0);
    while (nextCommand = commands.getCommand())
    {
        switch (nextCommand)
        {
        case GAME_COMMAND_TURN_LEFT:
            player->actor.rotateTowards(-1);
            break;
        case GAME_COMMAND_TURN_RIGHT:
            player->actor.rotateTowards(1);
            break;
        case GAME_COMMAND_MOVE_UP:
            relativeDirVector.y += 1.0f;
            break;
        case GAME_COMMAND_MOVE_DOWN:
            relativeDirVector.y -= 1.0f;
            break;
        case GAME_COMMAND_MOVE_LEFT:
            relativeDirVector.x -= 1.0f;
            break;
        case GAME_COMMAND_MOVE_RIGHT:
            relativeDirVector.x += 1.0f;
            break;
        case GAME_COMMAND_RUNNING_SPEED:
            player->actor.movementSpeed = runningMovementSpeed;
            break;
        case GAME_COMMAND_WALKING_SPEED:
            player->actor.movementSpeed = standardMovementSpeed;
            break;
        case GAME_COMMAND_SAVE_QUICK:
            saveGameToFile("save/quick", &currentGameState);
            break;
        case GAME_COMMAND_LOAD_QUICK:
            loadSaveFromFile("save/quick", &currentGameState);
            break;
        case GAME_COMMAND_TOGGLE_MINIMAP:
            currentGameState.showMiniMap = !currentGameState.showMiniMap;
            break;
        case GAME_COMMAND_TOGGLE_MOUSEMODE:
            toggleMouseMode();
            break;
        }
    }

    if (relativeMouseMode)
    {
        jadel::Vec2 mouseDelta = jadel::inputGetMouseDeltaRelative();
        player->actor.modifyRotation((-mouseDelta.x * mouseSensitivity));
    }
    if (relativeDirVector.length() > 0)
        relativeDirVector = relativeDirVector.normalize() * player->actor.movementSpeed;

    jadel::Mat3 playerRotationMatrix = getRotationMatrix(player->getRotation());
    jadel::Vec2 forward = playerRotationMatrix * relativeDirVector;
    Ray facingRay;
    facingRay.shoot(player->getPosition(), playerRotationMatrix * jadel::Vec2(0, 1), 100, &currentGameState.map);
    currentGameState.minimap.pushLine(facingRay.start, facingRay.end, {1, 0, 1, 0});
    if (jadel::inputIsMouseLeftHeld())
    {
        if (facingRay.getRayEndClip().content.barrier)
        {
            int mapX = facingRay.getRayEndClip().content.sectorX;
            int mapY = facingRay.getRayEndClip().content.sectorY;
            currentGameState.map.getSector(mapX, mapY)->texture = wallTex;
        }
    }
    player->actor.vel = forward;

    int mWheelScrolls = jadel::inputGetMouseWheelScrolls();
    if (mWheelScrolls)
    {
        mouseSensitivity += 5.0f * mWheelScrolls;
        jadel::message("%f\n", mouseSensitivity);
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

    if (jadel::inputIsKeyPressed(jadel::KEY_X))
    {
        modScreenPlane(0.8f * frameTime, 0);
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_Z))
    {
        modScreenPlane(-0.8f * frameTime, 0);
    }

    if (jadel::inputIsKeyPressed(jadel::KEY_V))
    {
        modScreenPlane(0, 0.8f * frameTime);
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_C))
    {
        modScreenPlane(0, -0.8f * frameTime);
    }

    player->actor.move();

    currentGameState.camera.setPosition(player->getPosition());
    currentGameState.camera.setRotation(player->getRotation());

    render();
}

bool createDefaultActor(float x, float y, Actor *target)
{
    if (!target)
        return false;
    target->dim = {0, 0, 0.3f, 0.7f};
    target->setPosition(x, y);
    target->setRotation(90.0f);
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
    initRenderer();
    if (!assetManager.init())
    {
        exit(0);
    }
    if (!assetManager.loadTexture("textures/myFace.png"))
    {
        exit(0);
    }
    if (!assetManager.loadTexture("textures/wall.png"))
    {
        exit(0);
    }
    setMouseMode(true);
    bool gameStateSet = false;
    bool mapLoaded = false;
    maxActors = 100;
    numActors = 0;
    Map map;

    actors = (Actor *)jadel::memoryReserve(maxActors * sizeof(Actor));
    pushActor(2.5f, 3.5f, {0, 0, 0.3f, 0.7f});
    pushActor(5.5f, 5.5f, {0, 0, 0.4f, 0.6f});
    setScreenPlane(0.15f, screenPlaneDist / aspectHeight);
    ControlScheme *gameControls = &currentGameState.controls;
    gameControls->init();
    gameControls->addKeyPressCommand(GAME_COMMAND_MOVE_UP, jadel::KEY_W);
    gameControls->addKeyPressCommand(GAME_COMMAND_MOVE_DOWN, jadel::KEY_S);
    gameControls->addKeyPressCommand(GAME_COMMAND_MOVE_LEFT, jadel::KEY_A);
    gameControls->addKeyPressCommand(GAME_COMMAND_MOVE_RIGHT, jadel::KEY_D);
    gameControls->addKeyPressCommand(GAME_COMMAND_TURN_LEFT, jadel::KEY_LEFT);
    gameControls->addKeyPressCommand(GAME_COMMAND_TURN_RIGHT, jadel::KEY_RIGHT);
    gameControls->addKeyTypeCommand(GAME_COMMAND_SAVE_QUICK, jadel::KEY_Q);
    gameControls->addKeyTypeCommand(GAME_COMMAND_LOAD_QUICK, jadel::KEY_E);
    gameControls->addKeyPressCommand(GAME_COMMAND_RUNNING_SPEED, jadel::KEY_SHIFT);
    gameControls->addKeyReleaseCommand(GAME_COMMAND_WALKING_SPEED, jadel::KEY_SHIFT);
    gameControls->addKeyTypeCommand(GAME_COMMAND_TOGGLE_MINIMAP, jadel::KEY_TAB);
    gameControls->addKeyTypeCommand(GAME_COMMAND_TOGGLE_MOUSEMODE, jadel::KEY_K);

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
        currentGameState.player.actor.setPosition(1.5f, 1.5f);
        currentGameState.player.actor.setRotation(0);
        currentGameState.showMiniMap = false;
    }
    // currentGameState.map.init(20, 15);
    if (!mapLoaded)
    {
        Map::loadFromFile("maps/map04.rcmap", &map);
    }
    currentGameState.setMap(&map);

    currentGameState.player.actor.movementSpeed = standardMovementSpeed;
    currentGameState.player.actor.turningSpeed = 140.0f;
    currentGameState.camera.setPosition(currentGameState.player.getPosition());
    currentGameState.camera.setRotation(currentGameState.player.getRotation());

    Minimap *minimap = &currentGameState.minimap;
    minimap->maxRects = 500;
    minimap->maxLines = 500;
    minimap->rects = (RectRenderable *)jadel::memoryReserve(minimap->maxRects * sizeof(RectRenderable));
    minimap->lines = (LineRenderable *)jadel::memoryReserve(minimap->maxLines * sizeof(LineRenderable));
    minimap->scale = 0.05f;
    minimap->screenStart = jadel::Vec2(0.2f, 0);
    minimap->clear();
    startEffectTimer.start();
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

    initImageLoad();
    jadel::Surface mapPNG;

    /*
        if (!load_PNG("maps/map4png.png", &mapPNG))
        {
            return 0;
        }

        Map map;
        map.init(mapPNG.width, mapPNG.height, "Fourth map");

        uint32 *mapPixels = (uint32 *)mapPNG.pixels;
        for (int y = 0; y < map.height; ++y)
        {
            for (int x = 0; x < map.width; ++x)
            {
                int index = x +  y * map.width;
                jadel::Color mapPixel = createColorFrom(mapPixels[x + y * map.width]);
                map.sectors[index].barrier = mapPixel.a > 0;
                map.sectors[index].color = mapPixel;
                map.sectors[index].texture = assetManager.findTexture("textures/myFace.png");
            }
        }
        map.saveToFile("maps/map04");
    */
    init();
    frameTimer.start();
    uint32 elapsedInMillis = 0;
    uint32 minFrameTime = 1000 / 165;
    uint32 accumulator = 0;
    int framesPerSecond = 0;

    while (true)
    {
        tick();
        JadelUpdate();
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