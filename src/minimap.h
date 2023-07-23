#pragma once
#include <jadel.h>
#include "graphics.h"

struct Minimap
{
    jadel::Vec2 screenStart;
    float scale;
    RectRenderable *rects;
    size_t maxRects;
    size_t numRects;
    LineRenderable *lines;
    size_t maxLines;
    size_t numLines;

    int pushRect(jadel::Rectf rect, jadel::Color color);

    int pushRect(jadel::Vec2 p0, jadel::Vec2 p1, jadel::Color color);

    int pushLine(jadel::Vec2 start, jadel::Vec2 end, jadel::Color color);

    jadel::Vec2 findPointOnScreen(jadel::Vec2 point) const;

    void clear();
};