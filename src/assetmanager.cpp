#include "assetmanager.h"
#include "imageload.h"

bool AssetManager::init()
{
    bool texturesInit = jadel::vectorInit(50, &this->textures);
    if (!texturesInit)
    {
        this->textures.freeVector();
        return false;
    }
    return true;
}

void AssetManager::destroy()
{
    for (Texture &tex : textures)
    {
        tex.freeTexture();
    }
    this->textures.freeVector();
}

Texture *AssetManager::loadTexture(const char *filepath)
{
    jadel::Surface surface;
    if (!load_PNG(filepath, &surface))
    {
        jadel::message("[ERROR] Could not load texture: %s\n", filepath);
        return NULL;
    }

    this->textures.push(Texture(surface, filepath));
    return &this->textures.back();
}

Texture *AssetManager::findTexture(const char *name)
{
    Texture *result = NULL;
    for (Texture& tex : textures)
    {
        if (tex.name == name)
        {
            result = &tex;
            break;
        }
    }
    if (!result)
    {
        jadel::message("Texture %s not in asset collection\n", name);
    }
    return result;
}

Texture *AssetManager::findOrLoadTexture(const char *filepath)
{
    Texture *result = findTexture(filepath);
    if (result)
    {
        return result;
    }
    jadel::message("Trying to load texture...\n");
    result = loadTexture(filepath);
    return result;
}