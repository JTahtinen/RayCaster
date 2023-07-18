#pragma once
#include <jadel.h>

struct PositionData
{
    jadel::Vec2 position;
    float rotation;
};

struct Actor
{
    PositionData positionData;
    jadel::Vec2 vel;
    jadel::Rectf dim;
    
    float movementSpeed;
    float turningSpeed;

    void setPosition(jadel::Vec2 pos);
    void setPosition(float x, float y);
    void setRotation(float val);
    void modifyRotation(float val);
    void rotateTowards(int dir);
    jadel::Vec2 getPosition() const;    
    float getRotation() const;
    void move();
    
};

struct Player
{
    Actor actor;

    jadel::Vec2 getPosition() const;
    float getRotation() const;
};

struct Camera
{
    PositionData positionData;

    void setPosition(jadel::Vec2 pos);
    void setPosition(float x, float y);
    void setRotation(float deg);
    jadel::Vec2 getPosition() const;
    float getRotation() const;
};