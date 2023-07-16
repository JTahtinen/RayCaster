#include "map.h"
#include "util.h"

static MapUnit nullMapUnit = {0};

static const char *const mapSignature = "RCMAP";

struct MapFile
{
    char signature[5];
    int nameLength;
    char name[MAX_MAP_NAME_LENGTH + 1];
    int fileNameLength;
    char fileName[MAX_MAP_FILENAME_LENGTH + 1];
    int width;
    int height;
    MapUnit *content;

    bool loadFromFile(Map* target, const char * const fileName);
    bool saveToFile(const char * const fileName) const;
};

MapUnit *MapUnit::getNullMapUnit()
{
    return &nullMapUnit;
}

static bool initWithoutPopulating(int width, int height, const char *const name, Map *target)
{
    if (width < 1 || height < 1)
    {
        target->sectors = NULL;
        target->width = 0;
        target->height = 0;
        return false;
    }
    target->sectors = (MapUnit *)jadel::memoryReserve(width * height * sizeof(MapUnit));
    if (!target->sectors)
    {
        jadel::message("[ERROR] Could init map - Memory allocation failed!\n");
        target->width = 0;
        target->height = 0;
        return false;
    }
    strncpy_s(target->name, MAX_MAP_NAME_LENGTH, name, MAX_MAP_NAME_LENGTH);
    target->width = width;
    target->height = height;
    target->isSavedToFile = false;
    return true;
}

bool init(MapFile *mapFile, Map* target)
{
    if (!target || !mapFile || !mapFile->content)
    {
        jadel::message("[ERROR] Could not init map - Null map file!\n");
        target->sectors = NULL;
        target->width = 0;
        target->height = 0;
        return false;
    }
    if (!initWithoutPopulating(mapFile->width, mapFile->height, mapFile->name, target))
    {
        return false;
    }
    strncpy_s(target->fileName, mapFile->fileNameLength + 1, mapFile->fileName, mapFile->fileNameLength + 1);
    for (int y = 0; y < target->height; ++y)
    {
        for (int x = 0; x < target->width; ++x)
        {
            target->setSectorContent(x, y, mapFile->content[x + y * target->width]);
        }
    }
    return true;
}

bool Map::init(int width, int height, const char *const name)
{
    initWithoutPopulating(width, height, name, this);
    memset(this->sectors, 0, sizeof(MapUnit) * this->width * this->height);
    return true;
}



void Map::free()
{
    jadel::memoryFree(this->sectors);
    this->sectors = NULL;
    this->width = 0;
    this->height = 0;
}

bool Map::isSectorInBounds(int x, int y) const
{
    bool result(x >= 0 && x < this->width && y >= 0 && y < this->height);
    return result;
}

const MapUnit *Map::getSectorContent(int x, int y) const
{
    if (!isSectorInBounds(x, y))
        return &nullMapUnit;
    return &this->sectors[x + y * this->width];
}

MapUnit *Map::getSectorContent(int x, int y)
{
    if (!isSectorInBounds(x, y))
        return &nullMapUnit;
    return &this->sectors[x + y * this->width];
}

void Map::setSectorContent(int x, int y, MapUnit content)
{
    MapUnit *unit = this->getSectorContent(x, y);
    if (!unit)
    {
        return;
    }
    *unit = content;
}

#define IF_FALSE_GOTO_FAIL(value, errMessage)                                            \
    if (!value)                                                                          \
    {                                                                                    \
        const char *errorMessage = errMessage;                                           \
        jadel::message("[ERROR] Could not load map: %s - %s\n", filePath, errorMessage); \
        goto maploadfail;                                                                \
    }

bool MapFile::loadFromFile(Map *target, const char *const fileName)
{
    *this = {0};
    jadel::BinaryFile file;
    const char *filePath;
    char *modifiedFileName = NULL;
    if (!isStringHeader(".rcmap", fileName))
    {
        modifiedFileName = appendString(fileName, ".rcmap");
        filePath = modifiedFileName;
    }
    else
    {
        filePath = fileName;
    }

    IF_FALSE_GOTO_FAIL(file.init(filePath), "File not found!");

    file.readString(this->signature, 5);

    IF_FALSE_GOTO_FAIL(strncmp(mapSignature, this->signature, 2) == 0,
                       "Invalid file type or corrupt file!");

    file.readInt(&this->nameLength);
    IF_FALSE_GOTO_FAIL((this->nameLength <= MAX_MAP_NAME_LENGTH), "Invalid map name!");
    file.readString(this->name, this->nameLength);

    file.readInt(&this->fileNameLength);
    IF_FALSE_GOTO_FAIL((this->fileNameLength <= MAX_MAP_FILENAME_LENGTH), "Invalid file name!");
    file.readString(this->fileName, this->fileNameLength);

    file.readInt(&this->width);
    file.readInt(&this->height);

    IF_FALSE_GOTO_FAIL((this->width > 0 && this->height > 0), "Invalid map size");

    this->content = (MapUnit *)jadel::memoryReserve(sizeof(MapUnit) * this->width * this->height);
    IF_FALSE_GOTO_FAIL(content, "Memory allocation failure!");

    IF_FALSE_GOTO_FAIL(file.readNBytes(this->content, sizeof(MapUnit) * this->width * this->height),
                       "Corrupt file: invalid map data!");

    IF_FALSE_GOTO_FAIL(init(this, target), "Memory allocation failure");

    jadel::memoryFree(this->content);
    target->isSavedToFile = true;
    return true;

maploadfail:
    file.close();
    return false;
}

#define CLEANUP_AND_RETURN(success)          \
    if (modifiedFileName)                    \
        jadel::memoryFree(modifiedFileName); \
    file.close();                            \
    return success

bool MapFile::saveToFile(const char *const fileName) const
{
    const char *finalFileName;
    char *modifiedFileName = NULL;
    if (isStringHeader(".rcmap", fileName))
    {
        finalFileName = fileName;
    }
    else
    {
        modifiedFileName = appendString(fileName, ".rcmap");
        finalFileName = modifiedFileName;
    }
    jadel::BinaryFile file;
    file.init(sizeof(MapFile) + (this->width * this->height * sizeof(MapUnit)));
    file.writeString(mapSignature);
    file.writeInt(this->nameLength);
    file.writeString(this->name, this->nameLength);
    file.writeInt(this->fileNameLength);
    file.writeString(this->fileName, this->fileNameLength);
    file.writeInt(this->width);
    file.writeInt(this->height);
    file.writeNBytes(this->width * this->height * sizeof(MapUnit), this->content);

    
    if (!file.writeToFile(finalFileName))
    {
        jadel::message("[ERROR] Could not save map file: %s\n", finalFileName);
        CLEANUP_AND_RETURN(false);
    }
    CLEANUP_AND_RETURN(true);
}

bool Map::saveToFile(const char *const fileName)
{
    if (!fileName) 
    {
        jadel::message("[ERROR] Could not save map - File name not specified!\n");
        return false;
    }
    char* modifiedFileName = NULL;
    const char* filePath = fileName;
    if (!isStringHeader(".rcmap", fileName))
    {
        modifiedFileName = appendString(fileName, ".rcmap");
        filePath = modifiedFileName;
    }
    int fileNameLength = strnlen_s(filePath, MAX_MAP_FILENAME_LENGTH + 1);
    if (fileNameLength > MAX_MAP_FILENAME_LENGTH)
    {
        jadel::message("[ERROR] Could not save map - File name too long!");
    }
    MapFile mapFile;
    memmove(mapFile.signature, mapSignature, strlen(mapSignature));
    mapFile.nameLength = strnlen_s(&this->name[0], MAX_MAP_NAME_LENGTH);
    strncpy_s(mapFile.name, MAX_MAP_NAME_LENGTH, this->name, MAX_MAP_NAME_LENGTH);
    mapFile.fileNameLength = fileNameLength;
    strncpy_s(mapFile.fileName, mapFile.fileNameLength + 1, filePath, mapFile.fileNameLength + 1);
    mapFile.width = this->width;
    mapFile.height = this->height;
    size_t contentSize = this->width * this->height * sizeof(MapUnit);
    mapFile.content = (MapUnit *)jadel::memoryReserve(contentSize);
    //jadel::memoryPrintDebugData();
    if (!mapFile.content)
    {
        jadel::message("[ERROR] Could not save map file: %s - Memory allocation failed!\n", fileName);
        if (modifiedFileName) jadel::memoryFree(modifiedFileName);
        return false;
    }
    //memmove(mapFile.content, this->sectors, contentSize);

    for (int i = 0; i < mapFile.width * mapFile.height; ++i)
    {
            mapFile.content[i] = this->sectors[i];
    }
    bool result = mapFile.saveToFile(fileName);
    jadel::memoryFree(mapFile.content);
    if (result)
    {
        if (!this->isSavedToFile)
        {
            strncpy_s(this->fileName, fileNameLength + 1, fileName, fileNameLength + 1);
        }
        this->isSavedToFile = true;
    }
    if (modifiedFileName) jadel::memoryFree(modifiedFileName);
    return result;
}

bool Map::saveToFile()
{
    if (!this->isSavedToFile)
    {
        jadel::message("[ERROR] Could not save map to file - No file name!\n");
        return false;
    }
    bool result = this->saveToFile(this->fileName);
    return result;
}

bool Map::loadFromFile(const char * const fileName, Map* map)
{
    MapFile file;
    bool result = file.loadFromFile(map, fileName);
    return result;
}