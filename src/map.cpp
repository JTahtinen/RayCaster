#include "map.h"
#include "util.h"
#include "rc.h"
#include "defs.h"

static Sector nullMapUnit = {0};

static const char *const mapSignature = "RCMAP";

struct MapFileSector
{
    bool barrier;
    jadel::Color color;
    char textureFilePath[MAX_TEXTURE_FILEPATH_LENGTH + 1];
};

static size_t mapFileHeaderSize = 0;

struct MapFileHeader
{
    char signature[6];
    char name[MAX_MAP_NAME_LENGTH + 1];
    char fileName[MAX_MAP_FILENAME_LENGTH + 1];
    int width;
    int height;
};

struct MapFile
{
    MapFileHeader header;
    MapFileSector *content;

    bool loadFromFile(Map *target, const char *const fileName);
    bool saveToFile(const char *const fileName) const;
};

Sector *Sector::getNullMapUnit()
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
    target->sectors = (Sector *)jadel::memoryReserve(width * height * sizeof(Sector));
    if (!target->sectors)
    {
        jadel::message("[ERROR] Could init map - Memory allocation failed!\n");
        target->width = 0;
        target->height = 0;
        return false;
    }
    strncpy(target->name, name, MAX_MAP_NAME_LENGTH);
    target->width = width;
    target->height = height;
    target->isSavedToFile = false;
    return true;
}

void setSectorContent(int x, int y, MapFileSector sector, Map *target)
{
    Sector *unit = target->getSector(x, y);
    if (!unit)
    {
        return;
    }
    unit->barrier = sector.barrier;
    unit->color = sector.color;
    unit->texture = assetManager.findTexture("textures/myFace.png");
}

bool init(const MapFile *mapFile, Map *target)
{
    if (!target || !mapFile || !mapFile->content)
    {
        jadel::message("[ERROR] Could not init map - Null map file!\n");
        target->sectors = NULL;
        target->width = 0;
        target->height = 0;
        return false;
    }
    if (!initWithoutPopulating(mapFile->header.width, mapFile->header.height, mapFile->header.name, target))
    {
        return false;
    }
    strncpy(target->fileName, mapFile->header.fileName, MAX_MAP_FILENAME_LENGTH);
    for (int y = 0; y < target->height; ++y)
    {
        for (int x = 0; x < target->width; ++x)
        {
            setSectorContent(x, y, mapFile->content[x + y * target->width], target);
        }
    }
    return true;
}

bool Map::init(int width, int height, const char *const name)
{
    initWithoutPopulating(width, height, name, this);
    memset(this->sectors, 0, sizeof(Sector) * this->width * this->height);
    return true;
}

void Map::freeMap()
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

const Sector *Map::getSectorContent(int x, int y) const
{
    if (!isSectorInBounds(x, y))
        return &nullMapUnit;
    return &this->sectors[x + y * this->width];
}

Sector *Map::getSector(int x, int y)
{
    if (!isSectorInBounds(x, y))
        return &nullMapUnit;
    return &this->sectors[x + y * this->width];
}

void Map::setSectorContent(int x, int y, Sector content)
{
    Sector *unit = this->getSector(x, y);
    if (!unit)
    {
        return;
    }
    *unit = content;
}

#define IF_FALSE_GOTO_CLEANUP(value, errMessage)                                         \
    if (!value)                                                                          \
    {                                                                                    \
        const char *errorMessage = errMessage;                                           \
        jadel::message("[ERROR] Could not load map: %s - %s\n", filePath, errorMessage); \
        result = false;                                                                  \
        goto cleanup;                                                                    \
    }

bool MapFile::loadFromFile(Map *target, const char *const fileName)
{
    bool result = true;
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

    IF_FALSE_GOTO_CLEANUP(file.init(filePath), "File not found!");

    file.readString(this->header.signature, sizeof(mapSignature), sizeof(mapSignature));

    IF_FALSE_GOTO_CLEANUP(strncmp(mapSignature, this->header.signature, sizeof(mapSignature)) == 0,
                          "Invalid file type or corrupt file!");

    file.readString(this->header.name, sizeof(this->header.name), sizeof(this->header.name));
    file.readString(this->header.fileName, sizeof(this->header.fileName), sizeof(this->header.fileName));
    file.readInt(&this->header.width);
    file.readInt(&this->header.height);

    IF_FALSE_GOTO_CLEANUP((this->header.width > 0 && this->header.height > 0), "Invalid map size");

    this->content = (MapFileSector *)jadel::memoryReserve(sizeof(MapFileSector) * this->header.width * this->header.height);
    IF_FALSE_GOTO_CLEANUP(content, "Memory allocation failure!");

    for (int numSectorsLoaded = 0; numSectorsLoaded < this->header.width * this->header.height; ++numSectorsLoaded)
    {
        MapFileSector *sector = &this->content[numSectorsLoaded];
        IF_FALSE_GOTO_CLEANUP(file.readBool(&sector->barrier),
                              "Corrupt file: invalid map data!");
        IF_FALSE_GOTO_CLEANUP(file.readFloat(&sector->color.a),
                              "Corrupt file: invalid map data!");
        IF_FALSE_GOTO_CLEANUP(file.readFloat(&sector->color.r),
                              "Corrupt file: invalid map data!");
        IF_FALSE_GOTO_CLEANUP(file.readFloat(&sector->color.g),
                              "Corrupt file: invalid map data!");
        IF_FALSE_GOTO_CLEANUP(file.readFloat(&sector->color.b),
                              "Corrupt file: invalid map data!");
        IF_FALSE_GOTO_CLEANUP(file.readString(sector->textureFilePath, sizeof(sector->textureFilePath), sizeof(sector->textureFilePath)),
                              "Corrupt file: invalid map data!");
    }
    IF_FALSE_GOTO_CLEANUP(init(this, target), "Initialization failure");

    target->isSavedToFile = true;
cleanup:
    jadel::memoryFree(this->content);
    file.close();
    return result;
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
    int fileNameLength;
    if (isStringHeader(".rcmap", fileName))
    {
        finalFileName = fileName; 
    }
    else
    {
        modifiedFileName = appendString(fileName, ".rcmap");
        finalFileName = modifiedFileName;
    }
    fileNameLength = strlen(finalFileName);
    if (fileNameLength > MAX_MAP_FILENAME_LENGTH)
    {
        jadel::message("[ERROR] Could not save map %s - Filename too long!\n", finalFileName);
        return false;
    }
    jadel::BinaryFile file;
    file.init(sizeof(MapFileHeader) + (this->header.width * this->header.height * sizeof(MapFileSector)));
    file.writeString(mapSignature, 5, sizeof(mapSignature));
    file.writeString(this->header.name, MAX_MAP_NAME_LENGTH, sizeof(this->header.name));
    file.writeString(finalFileName, MAX_MAP_FILENAME_LENGTH, sizeof(this->header.fileName));
    file.writeInt(this->header.width);
    file.writeInt(this->header.height);
    for (int i = 0; i < this->header.width * this->header.height; ++i)
    {
        MapFileSector* sector = &this->content[i];
        file.writeBool(sector->barrier);
        file.writeFloat(sector->color.a);
        file.writeFloat(sector->color.r);
        file.writeFloat(sector->color.g);
        file.writeFloat(sector->color.b);
        file.writeString(sector->textureFilePath, MAX_TEXTURE_FILEPATH_LENGTH, sizeof(sector->textureFilePath));
    }

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
    char *modifiedFileName = NULL;
    const char *filePath = fileName;
    if (!isStringHeader(".rcmap", fileName))
    {
        modifiedFileName = appendString(fileName, ".rcmap");
        filePath = modifiedFileName;
    }
    int fileNameLength = strnlen(filePath, MAX_MAP_FILENAME_LENGTH);
    if (fileNameLength > MAX_MAP_FILENAME_LENGTH)
    {
        jadel::message("[ERROR] Could not save map - File name too long!");
    }

    // Copy map data to map file
    MapFile mapFile;
    strncpy(mapFile.header.signature, mapSignature, sizeof(mapFile.header.signature) - 1);
    strncpy(mapFile.header.name, this->name, MAX_MAP_NAME_LENGTH);
    strncpy(mapFile.header.fileName, filePath, MAX_MAP_FILENAME_LENGTH);
    mapFile.header.width = this->width;
    mapFile.header.height = this->height;
    size_t contentSize = this->width * this->height * sizeof(MapFileSector);
    mapFile.content = (MapFileSector *)jadel::memoryReserve(contentSize);
    
    if (!mapFile.content)
    {
        jadel::message("[ERROR] Could not save map file: %s - Memory allocation failed!\n", fileName);
        if (modifiedFileName)
            jadel::memoryFree(modifiedFileName);
        return false;
    }

    for (int i = 0; i < mapFile.header.width * mapFile.header.height; ++i)
    {
        Sector *currentSector = &this->sectors[i];
        MapFileSector *currentFileSector = &mapFile.content[i];
        currentFileSector->barrier = currentSector->barrier;
        currentFileSector->color = currentSector->color;
        strncpy(currentFileSector->textureFilePath, currentSector->texture->name.c_str(), MAX_TEXTURE_FILEPATH_LENGTH);
    }
    // ********
    
    // Save mapFile
    bool result = mapFile.saveToFile(fileName);
    
    // Cleanup
    jadel::memoryFree(mapFile.content);
    if (result)
    {
        if (!this->isSavedToFile)
        {
            strncpy(this->fileName, fileName, MAX_MAP_FILENAME_LENGTH + 1);
        }
        this->isSavedToFile = true;
    }
    if (modifiedFileName)
        jadel::memoryFree(modifiedFileName);
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

bool Map::loadFromFile(const char *const fileName, Map *map)
{
    MapFile file;
    bool result = file.loadFromFile(map, fileName);
    return result;
}