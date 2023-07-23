#include "texture.h"

Texture::Texture(jadel::Surface surface, const char* name)
    : surface(surface)
    , name(name)
{
}

void Texture::freeTexture()
{
    jadel::memoryFree(this->surface.pixels);
    this->surface.pixels = NULL;
    this->name.free();
}