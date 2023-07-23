#pragma once
#include <jadel.h>
#include "texture.h"

struct AssetManager
{
    jadel::Vector<Texture> textures;
    
    AssetManager() = default;
    bool init();
    void destroy();
    Texture* loadTexture(const char* filepath);
    Texture* findTexture(const char* name);
    Texture* findOrLoadTexture(const char* filepath);
};