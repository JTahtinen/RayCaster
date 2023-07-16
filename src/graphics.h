#pragma once
#include <jadel.h>

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
