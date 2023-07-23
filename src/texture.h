#pragma once
#include <jadel.h>
#include "defs.h"

struct Texture
{
    jadel::String name;   
    jadel::Surface surface;

    Texture() = default;
    Texture(jadel::Surface surface, const char* name);
    void freeTexture();
};