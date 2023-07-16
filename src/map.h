#pragma once
#include <jadel.h>

struct MapUnit
{
    bool barrier;
    jadel::Color color;

    static MapUnit* getNullMapUnit();
};

#define MAX_MAP_NAME_LENGTH (20)
#define MAX_MAP_FILENAME_LENGTH (50)

struct Map
{
    char name[MAX_MAP_NAME_LENGTH + 1];
    char fileName[MAX_MAP_FILENAME_LENGTH + 1];
    MapUnit *sectors;
    int width;
    int height;
    bool isSavedToFile;

    bool init(int width, int height, const char * const name);
    void free();

    bool isSectorInBounds(int x, int y) const;
    const MapUnit* getSectorContent(int x, int y) const;
    MapUnit* getSectorContent(int x, int y);
    void setSectorContent(int x, int y, MapUnit content);
    bool saveToFile(const char * const fileName);
    bool saveToFile();
    static bool loadFromFile(const char * const fileName, Map* map);
};