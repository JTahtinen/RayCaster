#pragma once
#include <jadel.h>
#define MAX_KEY_COMMANDS_PER_TYPE (30)

struct KeyCommandList
{
    uint32 keys[MAX_KEY_COMMANDS_PER_TYPE];
    uint32 commands[MAX_KEY_COMMANDS_PER_TYPE];
    size_t numCommands;
};

struct ControlScheme
{
    KeyCommandList keyPressCommands;
    KeyCommandList keyTypeCommands;
    KeyCommandList keyReleaseCommands;

    void init();

    bool addKeyPressCommand(uint32 command, uint32 key);
    bool addKeyTypeCommand(uint32 command, uint32 key);
    bool addKeyReleaseCommand(uint32 command, uint32 key);

    size_t numCommands() const;
};