#include "util.h"
#include <string.h>
#include <jadel.h>
#include <math.h>
#include "defs.h"

bool isFloatEven(float val)
{
    bool result = (val - (float)((int)val)) == 0;
    return result;
}

bool isFloatEvenWithinMargin(float val, float margin)
{
    float remainder = fabs(val - roundf(val));
    bool result = remainder <= margin;
    //if (msg)
      //  jadel::message("Val: %f, margin: %f, remainder: %f, result: %d\n", val, margin, remainder, (int)result);
    return result;
}

bool isFloatEqualWithinMargin(float val, float comparison, float margin)
{
    float diff = val - comparison;
    bool result = fabs(diff) <= fabs(margin);
    return result;
}

bool isStringHeader(const char *expectedHeader, const char *string)
{
    size_t headerLength = strlen(expectedHeader);
    size_t stringLength = strlen(string);
    if (stringLength < headerLength)
        return false;

    for (size_t i = 0; i < 3; ++i)
    {
        if (string[stringLength - 1 - i] != expectedHeader[headerLength - 1 - i])
        {
            return false;
        }
    }
    return true;
}

char *appendString(const char *stringStart, size_t startLength, const char *stringEnd, size_t endLength)
{
    char *newString = (char *)jadel::memoryReserve(startLength + endLength + 1); // 4 characters for ".sv\0"
    if (!newString)
    {
        return NULL;
    }
    strncpy(newString, stringStart, startLength);
    strncpy(newString + startLength, stringEnd, endLength);
    newString[startLength + endLength] = '\0';
    return newString;
}

char *appendString(const char *const stringStart, const char *const stringEnd)
{
    size_t startLen = strlen(stringStart);
    size_t endLen = strlen(stringEnd);
    char *result = appendString(stringStart, startLen, stringEnd, endLen);
    return result;
}

float getAngleOfVec2(jadel::Vec2 vec)
{
    if (vec.x < 0)
    {
        return 360.0f + TO_DEGREES(atan2(vec.x, vec.y));
    }
    else
    {
        return TO_DEGREES(atan2(vec.x, vec.y));
    }
}

bool isVec2ALongerThanB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y) > (b.x * b.x + b.y * b.y);
    return result;
}

bool isVec2ALengthEqualToB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y) == (b.x * b.x + b.y * b.y);
    return result;
}

bool isVec2ALongerThanOrEqualToB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y) >= (b.x * b.x + b.y * b.y);
    return result;
}

float getCosBetweenVectors(jadel::Vec2 a, jadel::Vec2 b)
{
    float result = a.dot(b) / (a.length() + b.length());
    return result;
}

bool isValueBetween(float min, float max, float val)
{
    bool result = min < val && max >= val;
    return result;
}

float moduloFloat(float value, int mod)
{
    float result = value;
    if (!isValueBetween(0, (float)mod, value))
    {
        float remainder = value - (int)value;
        float fixedValue = (int)(roundf(value)) % mod + remainder;
        if (fixedValue < 0)
            fixedValue += (float)mod;
        result = fixedValue;
    }
    return result;
}

jadel::Vec2 clipSegmentFromLineAtY(jadel::Vec2 source, float y)
{
    if (source.y == 0)
        return source;
    float ratio = source.x / source.y;
    jadel::Vec2 result(ratio * y, y);
    return result;
}

jadel::Vec2 clipSegmentFromLineAtX(jadel::Vec2 source, float x)
{
    if (source.x == 0)
        return source;
    float ratio = source.y / source.x;
    jadel::Vec2 result(x, ratio * x);
    return result;
}

jadel::Mat3 getRotationMatrix(float angleInDegrees)
{
    float finalAngle = moduloFloat(angleInDegrees, 360);
    float radAngle = TO_RADIANS(finalAngle);
    jadel::Mat3 result =
        {
            cosf(radAngle), sinf(radAngle), 0,
            -sinf(radAngle), cosf(radAngle), 0,
            0, 0, 1};
    return result;
}