#include <jadel.h>
#include "saveload.h"
#include "util.h"
#include "rc.h"
#include <string.h>

static const char * const saveFileHeader = "SF";

struct SaveFile
{
    char header[sizeof(saveFileHeader)];
    GameState state;
};

bool loadSaveFromFile(const char *const filePath, GameState *target)
{
    jadel::BinaryFile file;
    const char *finalFilePath;
    char *modifiedFilePath = NULL;
    if (!isStringHeader(".sv", filePath))
    {
        modifiedFilePath = appendString(filePath, ".sv");
        finalFilePath = modifiedFilePath;
    }
    else
    {
        finalFilePath = filePath;
    }
    if (!target)
    {
        jadel::message("[ERROR] Could not load save file %s. No target file!\n", finalFilePath);
        return false;
    }
    if (!file.init(finalFilePath))
    {
        jadel::message("[ERROR] Could not load save file %s!\n", finalFilePath);
        return false;
    }
    char header[3] = {0};
    file.readString(header, sizeof(header), sizeof(header));

    if (strncmp(header, saveFileHeader, sizeof(saveFileHeader) - 1) != 0)
    {
        jadel::message("[ERROR] Could not load save file %s. Corrupt file!\n", finalFilePath);
        file.close();
        return false;
    }
    jadel::Vec2 playerPos;
    float playerRotation;
    int mapFilePathLength = 0;
    char mapFilePath[MAX_MAP_FILENAME_LENGTH + 1] = {0};
    file.readString(mapFilePath, MAX_MAP_FILENAME_LENGTH, sizeof(mapFilePath));
    file.readFloat(&playerPos.x);
    file.readFloat(&playerPos.y);
    file.readFloat(&playerRotation);
    file.readBool(&target->showMiniMap);
    target->player.actor.setPosition(playerPos);
    target->player.actor.setRotation(playerRotation);
    jadel::message("Loaded game: %s\n", finalFilePath);
    file.close();
    if (modifiedFilePath)
    {
        jadel::memoryFree(modifiedFilePath);
    }
    if (strncmp(mapFilePath, target->map.fileName, mapFilePathLength + 1) != 0)
    {
        Map map;
        Map::loadFromFile(mapFilePath, &map);
        currentGameState.setMap(&map);
    }
    return true;
}

bool saveGameToFile(const char *fileName, const GameState *state)
{
    if (!state->map.isSavedToFile)
    {
        jadel::message("[ERROR] Could not save game file: %s - Map file doesn't exist!\n", fileName);
        return false;
    }
    jadel::BinaryFile file;
    SaveFile saveFile;
    if (!file.init(sizeof(saveFile)))
    {
        jadel::message("[ERROR] Could not save game file: %s\n", fileName);
        return false;
    }

    const char *filePath;

    char *modifiedFilePath = NULL;
    if (isStringHeader(".sv", fileName))
    {
        filePath = fileName;
    }
    else
    {
        modifiedFilePath = appendString(fileName, ".sv");
        filePath = modifiedFilePath;
    }
    saveFile.header[0] = 'S';
    saveFile.header[1] = 'F';
    jadel::Vec2 playerPos = state->player.getPosition();
    float playerRotation = state->player.getRotation();
    file.writeString(saveFile.header, 2);
    file.writeString(state->map.fileName, MAX_MAP_FILENAME_LENGTH,sizeof(state->map.fileName));
    file.writeFloat(playerPos.x);
    file.writeFloat(playerPos.y);
    file.writeFloat(playerRotation);
    file.writeBool(state->showMiniMap);
    file.writeToFile(filePath);
    jadel::message("Saved game: %s\n", filePath);
    file.close();
    if (modifiedFilePath)
    {
        jadel::memoryFree(modifiedFilePath);
    }
    return true;
}