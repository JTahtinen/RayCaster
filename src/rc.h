#pragma once
#include <jadel.h>
#include "map.h"
#include "minimap.h"
#include "util.h"
#include "actor.h"
#include "controlscheme.h"
#include "assetmanager.h"

extern int screenWidth;
extern int screenHeight;

extern float aspectWidth;
extern float aspectHeight;

extern float frameTime;

enum
{
    GAME_COMMAND_MOVE_LEFT = 1,
    GAME_COMMAND_MOVE_RIGHT,
    GAME_COMMAND_MOVE_UP,
    GAME_COMMAND_MOVE_DOWN,
    GAME_COMMAND_TURN_LEFT,
    GAME_COMMAND_TURN_RIGHT,
    GAME_COMMAND_RUNNING_SPEED,
    GAME_COMMAND_WALKING_SPEED,
    GAME_COMMAND_TOGGLE_MINIMAP,
    GAME_COMMAND_SAVE_QUICK,
    GAME_COMMAND_LOAD_QUICK,
    GAME_COMMAND_TOGGLE_MOUSEMODE,
};

extern bool relativeMouseMode;

extern AssetManager assetManager;

struct GameState
{
    Map map;
    Minimap minimap;
    Player player;
    bool showMiniMap;
    Camera camera;
    ControlScheme controls;
    void setMap(Map *map);
};

extern GameState currentGameState;