#pragma once
#include <jadel.h>
#include "map.h"

extern int screenWidth;
extern int screenHeight;

extern float aspectWidth;
extern float aspectHeight;

extern float frameTime;

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
    Map map;
    Player player;
    bool showMiniMap;

    void setMap(Map *map);
};

extern GameState currentGameState;