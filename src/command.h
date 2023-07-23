#pragma once
#include "controlscheme.h"
#include "stackarray.h"
#include <jadel.h>

#define RC_COMMAND_NULL (0)

struct CommandList
{
    jadel::StackArray<uint32, MAX_KEY_COMMANDS_PER_TYPE * 3> list;
    size_t currentCommandIndex;

    void clear();
    uint32 getCommand();

    void push(uint32 command);
    void update(const ControlScheme *controls);
    inline uint32 operator[](size_t index) const
    {
        uint32 result = this->list[index];
        return result;
    }
};
