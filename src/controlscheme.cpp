#include "controlscheme.h"
#include "command.h"

void ControlScheme::init()
{
    this->keyPressCommands.numCommands = 0;
    this->keyTypeCommands.numCommands = 0;
    this->keyReleaseCommands.numCommands = 0;
}

static bool addKeyCommand(uint32 command, uint32 key, KeyCommandList *keyCommandList)
{
    if (command == RC_COMMAND_NULL)
    {
        return false;
    }
    keyCommandList->keys[keyCommandList->numCommands] = key;
    keyCommandList->commands[keyCommandList->numCommands] = command;
    ++keyCommandList->numCommands;
    return true;
}

bool ControlScheme::addKeyPressCommand(uint32 command, uint32 key)
{
    return addKeyCommand(command, key, &this->keyPressCommands);
}

bool ControlScheme::addKeyTypeCommand(uint32 command, uint32 key)
{
    return addKeyCommand(command, key, &this->keyTypeCommands);
}

bool ControlScheme::addKeyReleaseCommand(uint32 command, uint32 key)
{
    return addKeyCommand(command, key, &this->keyReleaseCommands);
}

size_t ControlScheme::numCommands() const
{
    size_t result = this->keyPressCommands.numCommands + this->keyTypeCommands.numCommands + this->keyReleaseCommands.numCommands;
    return result;
}