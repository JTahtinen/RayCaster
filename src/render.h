#pragma once

#include "graphics.h"

extern float internalHorizontalResolution;
extern int resolutionDivider;
extern float screenPlaneDist;
extern float screenPlaneWidth;

void setScreenPlane(float distance, float width);
void modScreenPlane(float distance, float width);
void render();