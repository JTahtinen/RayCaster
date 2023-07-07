#pragma once

#include <jadel.h>
#ifdef DEBUG

enum
{
    DEBUG_DATA_NOT_REGISTERED,
    DEBUG_DATA_REGISTERED
};

struct DebugData;
extern bool _DEBUGRegisterDebugData(DebugData* data);
extern bool _DEBUGInit();

struct DebugData
{
    int registered = DEBUG_DATA_NOT_REGISTERED;
    int handle;
};

void _DEBUGPushInt(int val, const char* identifier);
void _DEBUGPushFloat(float val, const char* identifier);
int _DEBUGGetInt(size_t index, const char* identifier);
float _DEBUGGetFloat(size_t index,  const char* identifier);
size_t _DEBUGGetNumInts(const char* identifier);
size_t _DEBUGGetNumFloats(const char* identifier);
void _DEBUGPrint(const char* identifier);
void _DEBUGClear(const char* identifier);


#define DEBUGInit() _DEBUGInit()
#define DEBUGPushInt(val, identifier) _DEBUGPushInt(val, identifier)
#define DEBUGPushFloat(val, identifier) _DEBUGPushFloat(val, identifier)
#define DEBUGGetInt(index, identifier) _DEBUGGetInt(index, identifier)
#define DEBUGGetFloat(index, identifier) _DEBUGGetFloat(index, identifier)
#define DEBUGGetNumInts(identifier) _DEBUGGetNumInts(identifier)
#define DEBUGGetNumFloats(identifier) _DEBUGGetNumFloats(identifier)
#define DEBUGPrint(identifier) _DEBUGPrint(identifier);
#define DEBUGClear(identifier) _DEBUGClear(identifier)

#else
#define DEBUGInit()
#define DEBUGPushInt(val, identifier)
#define DEBUGPushFloat(val, identifier)
#define DEBUGGetInt(index, identifier)
#define DEBUGGetFloat(index, identifier)
#define DEBUGGetNumInts(identifier)
#define DEBUGGetNumFloats(identifier)
#define DEBUGPrint(identifier)
#define DEBUGClear(identifier)
#endif