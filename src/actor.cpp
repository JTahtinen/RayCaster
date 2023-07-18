#include "actor.h"
#include "util.h"
#include "rc.h"
#include "ray.h"
#include "collision.h"

void Actor::setPosition(jadel::Vec2 pos)
{
    this->positionData.position = pos;
}

void Actor::setPosition(float x, float y)
{
    this->setPosition(jadel::Vec2(x, y));
}

void Actor::setRotation(float val)
{
    this->positionData.rotation = moduloFloat(val, 360);
}

void Actor::modifyRotation(float val)
{
    this->setRotation(this->getRotation() + val);
}

void Actor::rotateTowards(int dir)
{
    float finalDir = dir > 0 ? -1.0f : 1.0f;
    this->modifyRotation(finalDir * this->turningSpeed * frameTime);
}

jadel::Vec2 Actor::getPosition() const
{
    return this->positionData.position;
}

float Actor::getRotation() const
{
    return this->positionData.rotation;
}

void Actor::move()
{
    CollisionData collision;
    collision.check(&currentGameState.map, this->getPosition(), this->vel * frameTime);
    this->positionData.position += collision.checkedVelocity;
}

jadel::Vec2 Player::getPosition() const
{
    return this->actor.getPosition();
}

float Player::getRotation() const
{
    return this->actor.getRotation();
}

void Camera::setPosition(jadel::Vec2 pos)
{
    this->positionData.position = pos;
}

void Camera::setPosition(float x, float y)
{
    this->setPosition(jadel::Vec2(x, y));
}

void Camera::setRotation(float deg)
{
    this->positionData.rotation = deg;
}

jadel::Vec2 Camera::getPosition() const
{
    return positionData.position;
}

float Camera::getRotation() const
{
    return positionData.rotation;
}