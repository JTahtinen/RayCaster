#pragma once
#include <jadel.h>
#include "texture.h"

struct Sector
{
    bool barrier;
    const Texture* texture;
    jadel::Color color;

    static Sector* getNullMapUnit();
};

#define MAX_MAP_NAME_LENGTH (20)
#define MAX_MAP_FILENAME_LENGTH (50)

struct Map
{
    char name[MAX_MAP_NAME_LENGTH + 1];
    char fileName[MAX_MAP_FILENAME_LENGTH + 1];
    Sector *sectors;
    int width;
    int height;
    bool isSavedToFile;

    bool init(int width, int height, const char * const name);
    void freeMap();

    bool isSectorInBounds(int x, int y) const;
    const Sector* getSectorContent(int x, int y) const;
    Sector* getSector(int x, int y);
    void setSectorContent(int x, int y, Sector content);
    bool saveToFile(const char * const fileName);
    bool saveToFile();
    static bool loadFromFile(const char * const fileName, Map* map);
};