#pragma once
#include <jadel.h>

#define IS_VAL_BETWEEN( val, min, max ) ((min < val) && (val < max))
#define IS_VAL_BETWEEN_INCLUSIVE( val, min, max ) ((min <= val) && (val <= max))

bool isFloatEven(float val, float epsilon);

bool compareFloats(float val0, float val1, float epsilon);

bool compareVec2(jadel::Vec2 v0, jadel::Vec2 v1, float epsilon);

bool isStringHeader(const char *expectedHeader, const char *string);

char *appendString(const char *stringStart, size_t startLength, const char *stringEnd, size_t endLength);

char *appendString(const char *const stringStart, const char *const stringEnd);

float getAngleOfVec2(jadel::Vec2 vec);

bool isVec2ALongerThanB(jadel::Vec2 a, jadel::Vec2 b);

bool isVec2ALengthEqualToB(jadel::Vec2 a, jadel::Vec2 b);

bool isVec2ALongerThanOrEqualToB(jadel::Vec2 a, jadel::Vec2 b);

float getCosBetweenVectors(jadel::Vec2 a, jadel::Vec2 b);

bool isValueBetween(float min, float max, float val);

float moduloFloat(float value, int mod);

jadel::Vec2 clipSegmentFromLineAtY(jadel::Vec2 source, float y);

jadel::Vec2 clipSegmentFromLineAtX(jadel::Vec2 source, float x);

jadel::Mat3 getRotationMatrix(float angleInDegrees);