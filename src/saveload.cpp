#include <jadel.h>
#include "saveload.h"
#include "util.h"
#include "rc.h"
#include <string.h>

struct SaveFile
{
    char header[2];
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
        jadel::message("[ERROR] Could not load save file %s.!\n", finalFilePath);
        return false;
    }
    char header[3] = {0};
    file.readString(header, 2);

    if (header[0] != 'S' || header[1] != 'F')
    {
        jadel::message("[ERROR] Could not load save file %s. Corrupt file!\n", finalFilePath);
        file.close();
        return false;
    }
    int mapFilePathLength = 0;
    char mapFilePath[MAX_MAP_FILENAME_LENGTH + 1] = {0};
    file.readInt(&mapFilePathLength);
    file.readString(mapFilePath, mapFilePathLength);
    file.readFloat(&target->player.actor.pos.x);
    file.readFloat(&target->player.actor.pos.y);
    file.readFloat(&target->player.actor.facingAngle);
    file.readBool(&target->showMiniMap);
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
    if (state->map.isSavedToFile)
    {
        jadel::message("[ERROR] Could not save game file: %s - Map file doesn't exist!\n", fileName);
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
    file.writeString(saveFile.header, 2);
    file.writeInt(strnlen_s(state->map.fileName, MAX_MAP_FILENAME_LENGTH));
    file.writeString(state->map.fileName);
    file.writeFloat(state->player.actor.pos.x);
    file.writeFloat(state->player.actor.pos.y);
    file.writeFloat(state->player.actor.facingAngle);
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