#include "debug.h"

#ifdef DEBUG
#define NUM_INITIAL_DEBUG_DATA_PER_TYPE (20)
#define NUM_INITIAL_REGISTERED_DEBUG_DATA (50)

static int numRegisteredIdentifiers = 0;
static int poolSize = 0;

static jadel::Vector<jadel::String> debugIdentifierPool;
static jadel::Vector<int> *debugIntVectorPool = NULL;
static jadel::Vector<float> *debugFloatVectorPool = NULL;

void _DEBUGFreeRightFromIndexInclusive(int index)
{
    if (poolSize < 1 || index < poolSize - 1)
        return;
    for (int i = index; i < poolSize; ++i)
    {
        debugIdentifierPool[i].free();
        debugIntVectorPool[i].freeVector();
        debugFloatVectorPool[i].freeVector();
    }
}

bool _DEBUGSetPoolSize(int newSize)
{
    if (newSize < 1)
        return false;
    if (newSize == poolSize)
        return true;
    if (poolSize > 0)
    {
        if (!debugIntVectorPool || !debugFloatVectorPool)
            poolSize = 0;
    }
    int sizeDiff = newSize - poolSize;
    jadel::Vector<int> *newIntVectorPool;
    jadel::Vector<float> *newFloatVectorPool;
    newIntVectorPool = (jadel::Vector<int> *)jadel::memoryReserve(newSize * sizeof(jadel::Vector<int>));
    newFloatVectorPool = (jadel::Vector<float> *)jadel::memoryReserve(newSize * sizeof(jadel::Vector<float>));
    if (!newIntVectorPool || !newFloatVectorPool)
    {
        if (poolSize != 0)
            debugIdentifierPool.freeVector();
        
        jadel::memoryFree(debugIntVectorPool);
        jadel::memoryFree(debugFloatVectorPool);
        return false;
    }
    if (poolSize == 0)
    {
        if (!jadel::vectorInit(newSize, &debugIdentifierPool))
            return false;
    }

    int numElementsToCopy = poolSize;
    if (sizeDiff < 0)
    {
        numElementsToCopy = newSize;
        _DEBUGFreeRightFromIndexInclusive(poolSize - newSize - 1);
    }
    memmove(newIntVectorPool, debugIntVectorPool, numElementsToCopy * sizeof(jadel::Vector<int>));
    memmove(newFloatVectorPool, debugFloatVectorPool, numElementsToCopy * sizeof(jadel::Vector<float>));
    jadel::message("[DEBUG] Copied %d elements from old pool to new pool\n",numElementsToCopy);
    size_t numInitializedIntVectors = 0;
    size_t numInitializedFloatVectors = 0;
    size_t numInitializedStrings = 0;

    for (size_t i = poolSize; i < newSize; ++i)
    {
        jadel::String emptyIdentifier(100);
        if (!debugIdentifierPool.push(emptyIdentifier))
            goto fail;
        ++numInitializedStrings;
        if (!jadel::vectorInit(NUM_INITIAL_DEBUG_DATA_PER_TYPE, &newIntVectorPool[i]))
            goto fail;
        ++numInitializedIntVectors;
        if (!jadel::vectorInit(NUM_INITIAL_DEBUG_DATA_PER_TYPE, &newFloatVectorPool[i]))
            goto fail;
        ++numInitializedFloatVectors;
    }
    debugIdentifierPool.resize(newSize);
    jadel::memoryFree(debugIntVectorPool);
    jadel::memoryFree(debugFloatVectorPool);
    debugIntVectorPool = newIntVectorPool;
    debugFloatVectorPool = newFloatVectorPool;
    poolSize = newSize;
    jadel::message("[DEBUG] New pool size is %d\n", poolSize);
    return true;
fail:
    for (size_t i = 0; i < numInitializedStrings; ++i)
    {
        debugIdentifierPool[poolSize + i].free();
    }
    for (size_t i = 0; i < numInitializedIntVectors; ++i)
    {
        debugIntVectorPool[poolSize + i].freeVector();
    }
    for (size_t i = 0; i < numInitializedFloatVectors; ++i)
    {
        debugFloatVectorPool[poolSize + i].freeVector();
    }
    jadel::memoryFree(debugIntVectorPool);
    jadel::memoryFree(debugFloatVectorPool);
    debugIdentifierPool.freeVector();
    debugIntVectorPool = NULL;
    debugFloatVectorPool = NULL;
    return false;
}

bool _DEBUGInit()
{
    if (!JadelIsInitialized())
    {
        return false;
    }
    bool result = _DEBUGSetPoolSize(NUM_INITIAL_REGISTERED_DEBUG_DATA);

    return result;
}

bool _DEBUGDoublePoolSize()
{
    jadel::message("[DEBUG] Doubling pool size\n");
    return _DEBUGSetPoolSize(poolSize * 2);
}

void _DEBUGQuit()
{
    for (size_t i = 0; i < NUM_INITIAL_REGISTERED_DEBUG_DATA; ++i)
    {
        debugIntVectorPool[i].freeVector();
        debugFloatVectorPool[i].freeVector();
        debugIntVectorPool = NULL;
        debugFloatVectorPool = NULL;
    }
}

int _DEBUGFindIdentifierIndex(const char *identifier)
{
    for (int i = 0; i < debugIdentifierPool.size; ++i)
    {
        if (debugIdentifierPool[i] == identifier)
        {
            return i;
        }
    }
    return -1;
}

int _DEBUGRegisterIdentifier(const char *identifier)
{
    if (numRegisteredIdentifiers == debugIdentifierPool.size)
    {
        if (!_DEBUGDoublePoolSize())
        {
            return -1;
        }
    }
    int index = numRegisteredIdentifiers++;
    debugIdentifierPool[index].set(identifier);
    return index;
}

int _DEBUGFindOrRegisterIdentifier(const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex == -1)
    {
        identifierIndex = _DEBUGRegisterIdentifier(identifier);
    }
    return identifierIndex;
}

void _DEBUGPushInt(int val, const char *identifier)
{
    int identifierIndex = _DEBUGFindOrRegisterIdentifier(identifier);
    if (identifierIndex == -1)
    {
        return;
    }
    debugIntVectorPool[identifierIndex].push(val);
}

void _DEBUGPushFloat(float val, const char *identifier)
{
    int identifierIndex = _DEBUGFindOrRegisterIdentifier(identifier);
    if (identifierIndex == -1)
    {
        return;
    }
    debugFloatVectorPool[identifierIndex].push(val);
}

int _DEBUGGetInt(size_t index, int identifierIndex)
{
    if (identifierIndex >= numRegisteredIdentifiers)
        return -1;
    int result = debugIntVectorPool[identifierIndex][index];
    return result;
}

float _DEBUGGetFloat(size_t index, int identifierIndex)
{
    if (identifierIndex >= numRegisteredIdentifiers)
        return -1;
    float result = debugFloatVectorPool[identifierIndex][index];
    return result;
}

int _DEBUGGetInt(size_t index, const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex == -1)
        return -1;
    int result = _DEBUGGetInt(index, identifierIndex);
    return result;
}

float _DEBUGGetFloat(size_t index, const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex == -1)
        return -1;
    float result = _DEBUGGetFloat(index, identifierIndex);
    return result;
}

size_t _DEBUGGetNumInts(int index)
{
    if (index >= numRegisteredIdentifiers)
        return 0;
    size_t result = debugIntVectorPool[index].size;
    return result;
}

size_t _DEBUGGetNumFloats(int index)
{
    if (index >= numRegisteredIdentifiers)
        return 0;
    size_t result = debugFloatVectorPool[index].size;
    return result;
}

size_t _DEBUGGetNumInts(const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex == -1)
        return 0;
    size_t result = _DEBUGGetNumInts(identifierIndex);
    return result;
}

size_t _DEBUGGetNumFloats(const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex == -1)
        return 0;
    size_t result = _DEBUGGetNumFloats(identifierIndex);
    return result;
}

void _DEBUGPrint(const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex == -1)
        return;

    size_t numInts = _DEBUGGetNumInts(identifierIndex);
    size_t numFloats = _DEBUGGetNumFloats(identifierIndex);
    if (numInts == 0 && numFloats == 0)
        return;
    jadel::message("%s DEBUG DATA:\n", identifier);

    jadel::message("%d integers\n", numInts);
    for (size_t i = 0; i < numInts; ++i)
    {
        jadel::message("%d: %d\n", i, _DEBUGGetInt(i, identifierIndex));
    }
    jadel::message("\n%d floats\n", numFloats);
    for (size_t i = 0; i < numFloats; ++i)
    {
        jadel::message("%d: %f\n", i, _DEBUGGetFloat(i, identifierIndex));
    }
    jadel::message("---------\n");
}

void _DEBUGClear(const char *identifier)
{
    int identifierIndex = _DEBUGFindIdentifierIndex(identifier);
    if (identifierIndex < -1)
        return;
    debugIntVectorPool[identifierIndex].clear();
    debugFloatVectorPool[identifierIndex].clear();
}

#endif