#include "minimap.h"
#include <jadel.h>
#include "rc.h"

    int Minimap::pushRect(jadel::Rectf rect, jadel::Color color)
    {
        if (numRects >= maxRects)
            return 0;
        rects[numRects++] = {rect, color};
        return 1;
    }

    int Minimap::pushRect(jadel::Vec2 p0, jadel::Vec2 p1, jadel::Color color)
    {
        return pushRect(jadel::Rectf(p0, p1), color);
    }

    int Minimap::pushLine(jadel::Vec2 start, jadel::Vec2 end, jadel::Color color)
    {
        if (numLines >= maxLines)
            return 0;
        this->lines[this->numLines++] = {start, end, color};
        return 1;
    }

    jadel::Vec2 Minimap::findPointOnScreen(jadel::Vec2 point) const
    {
        jadel::Vec2 result = this->screenStart + (jadel::Vec2(point.x * aspectHeight, point.y) * this->scale);
        return result;
    }

    void Minimap::clear()
    {
        this->numRects = 0;
        this->numLines = 0;
    }