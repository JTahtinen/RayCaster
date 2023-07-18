#include "command.h"

uint32 CommandList::getCommand()
{
    if (this->list.empty() || this->currentCommandIndex >= this->list.size)
    {
        return RC_COMMAND_NULL;
    }
    uint32 result = this->list[this->currentCommandIndex++];
    return result;
}

void CommandList::clear()
{
    this->list.clear();
    this->currentCommandIndex = 0;
}

void CommandList::push(uint32 command)
{
    this->list.push(command);
}

void CommandList::update(const ControlScheme *controls)
{
    if (!controls)
    {
        return;
    }
    this->clear();
    for (size_t i = 0; i < controls->keyPressCommands.numCommands; ++i)
    {
        if (jadel::inputIsKeyPressed(controls->keyPressCommands.keys[i]))
        {
            this->push(controls->keyPressCommands.commands[i]);
        }
    }
    for (size_t i = 0; i < controls->keyTypeCommands.numCommands; ++i)
    {
        if (jadel::inputIsKeyTyped(controls->keyTypeCommands.keys[i]))
        {
            this->push(controls->keyTypeCommands.commands[i]);
        }
    }
    for (size_t i = 0; i < controls->keyReleaseCommands.numCommands; ++i)
    {
        if (jadel::inputIsKeyReleased(controls->keyReleaseCommands.keys[i]))
        {
            this->push(controls->keyReleaseCommands.commands[i]);
        }
    }
}