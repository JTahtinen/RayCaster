#include <jadel.h>
#include <thread>
#include "timer.h"

#define MATH_PI (3.141592653)
#define TO_RADIANS(deg) (deg * MATH_PI / 180.0)
#define TO_DEGREES(rad) (rad * (180.0 / MATH_PI))

static int screenWidth = 1280;
static int screenHeight = 720;

static float aspectWidth = (float)screenWidth / (float)screenHeight;
static float aspectHeight = (float)screenHeight / (float)screenWidth;

static float frameTime;

#define MAP_WIDTH (20)
#define MAP_HEIGHT (15)

#define MAX_RAY_DISTANCE (10)

static int map[MAP_WIDTH * MAP_HEIGHT];

struct Line
{
    jadel::Vec2 start;
    jadel::Vec2 end;
    jadel::Color color;
};

struct RectRenderable
{
    jadel::Rectf rect;
    jadel::Color color;
};

#define MAX_LINE_RENDERABLES (500)
#define MAX_RECT_RENDERABLES (500)

Line lineRenderables[MAX_LINE_RENDERABLES];
int numLineRenderables;

RectRenderable rectRenderables[MAX_RECT_RENDERABLES];
int numRectRenderables;

jadel::Vec2 camPos(1, 1);
jadel::Vec2 lastCamPos(1, 1);

void pushLineRenderable(jadel::Vec2 start, jadel::Vec2 end, jadel::Color color)
{
    if (numLineRenderables < MAX_LINE_RENDERABLES)
        lineRenderables[numLineRenderables++] = {start, end, color};
}

void pushRectRenderable(jadel::Rectf rect, jadel::Color color)
{
    if (numRectRenderables < MAX_RECT_RENDERABLES)
        rectRenderables[numRectRenderables++] = {rect, color};
}

jadel::Vec2 clipVectorAtY(jadel::Vec2 source, float y)
{
    if (source.y == 0)
        return source;
    float ratio = source.x / source.y;
    jadel::Vec2 result(ratio * y, y);
    return result;
}

jadel::Vec2 clipVectorAtX(jadel::Vec2 source, float x)
{
    if (source.x == 0)
        return source;
    float ratio = source.y / source.x;
    jadel::Vec2 result(x, ratio * x);
    return result;
}

int getSectorContent(int x, int y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return 0;
    return map[x + y * MAP_WIDTH];
}

int getSectorContent(float x, float y)
{
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return 0;
    return getSectorContent((int)x, (int)y);
}

bool isVec2ALongerThanB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y > b.x * b.x + b.y * b.y);
    return result;
}

bool isVec2ALengthEqualToB(jadel::Vec2 a, jadel::Vec2 b)
{
    bool result = (a.x * a.x + a.y * a.y == b.x * b.x + b.y * b.y);
    return result;
}

float getCosBetweenVectors(jadel::Vec2 a, jadel::Vec2 b)
{
    float result = a.dot(b) / (a.length() + b.length());
    return result;
}

static float camRotation = 0;

jadel::Mat3 cameraRotationMatrix;
static float screenPlaneDist = 0.15f;
static float screenPlaneWidth = screenPlaneDist / aspectHeight;
void render()
{
    numLineRenderables = 0;
    numRectRenderables = 0;
    jadel::graphicsClearTargetSurface();

    jadel::graphicsDrawRectRelative({-1, -1, 1, 0}, {1, 0.3f, 0.4f, 0.25f});
    jadel::graphicsDrawRectRelative({-1, 0, 1, 1}, {1, 0.2f, 0.2f, 0.4f});
    const float edgeXDiff = 1.0f;
    float columnWidth = (2.0f * edgeXDiff) / (float)screenWidth;
    static jadel::Vec2 mapStart(0.2f, 0);

    jadel::Vec2 unRotatedScreenStartClip(-screenPlaneWidth * 0.5f, screenPlaneDist);
    jadel::Vec2 unRotatedScreenEndClip(screenPlaneWidth * 0.5f, screenPlaneDist);
    jadel::Vec2 screenWorldStart = camPos + cameraRotationMatrix.mul(unRotatedScreenStartClip);
    jadel::Vec2 screenWorldEnd = camPos + cameraRotationMatrix.mul(unRotatedScreenEndClip);
    float screenPlaneXEnd = edgeXDiff;

    for (int seg = 0; seg < screenWidth; ++seg)
    {
        float currentScreenX = -edgeXDiff + (float)seg * columnWidth;
        jadel::Vec2 unRotatedRayLine(unRotatedScreenStartClip.x + (float)seg * (screenPlaneWidth / (float)screenWidth), screenPlaneDist); // Can be modified, so x should not be set to currentScreenX
        // unRotatedRayLine = clipVectorAtY(unRotatedRayLine, 1.0f);
        jadel::Vec2 currentRayLine = cameraRotationMatrix.mul(unRotatedRayLine);
        float rayXDir = currentRayLine.x > 0 ? 1.0f : -1.0f;
        float rayYDir = currentRayLine.y > 0 ? 1.0f : -1.0f;
        jadel::Vec2 clippedRaySegment(0, 0);

        // jadel::Vec2 unRotatedScreenClip = clipVectorAtY(unRotatedRayLine, defaultScreenPlane.y);
        // jadel::Vec2 rotatedScreenClip = cameraRotationMatrix.mul(unRotatedScreenClip);

        jadel::Vec2 rayStartPos = camPos + currentRayLine;
        jadel::Vec2 rayMarchPos = rayStartPos;

        for (int i = 0; i < MAX_RAY_DISTANCE; ++i)
        {
            bool isVerticallyClipped = false;
            bool isClippedOnCorner = false;
            float RayEndDistToUpperSector = (float)((int)(rayMarchPos.y + 1.0f)) - rayMarchPos.y;
            float rayEndDistToLowerSector = (float)(int)(rayMarchPos.y) - rayMarchPos.y;
            float rayEndDistToRightSector = (float)((int)(rayMarchPos.x + 1.0f)) - rayMarchPos.x;
            float rayEndDistToLeftSector = (float)((int)rayMarchPos.x) - rayMarchPos.x;

            if (rayEndDistToLowerSector == 0.0f)
                rayEndDistToLowerSector = -1.0f;
            if (rayEndDistToLeftSector == 0.0f)
                rayEndDistToLeftSector = -1.0f;

            float rayEndDistToVerticalSector;
            float rayEndDistToHorizontalSector;

            /*
            Cut a segment from a line whose slope is the same as the ray's,
            and whose length covers the distance to the next sector.
            */
            if (currentRayLine.y > 0)
            {
                rayEndDistToVerticalSector = RayEndDistToUpperSector;
            }
            else
            {
                rayEndDistToVerticalSector = rayEndDistToLowerSector;
            }
            if (currentRayLine.x > 0)
            {
                rayEndDistToHorizontalSector = rayEndDistToRightSector;
            }
            else
            {
                rayEndDistToHorizontalSector = rayEndDistToLeftSector;
            }

            /*
                Compare the length of the ray that clips with the next sector in the X-axis
                with the one that clips with the Y-axis, choose the shorter one, and add it
                to the existing ray
            */
            jadel::Vec2 verticallyClippedRay = clipVectorAtY(currentRayLine, rayEndDistToVerticalSector);
            jadel::Vec2 horizontallyClippedRay = clipVectorAtX(currentRayLine, rayEndDistToHorizontalSector);

            if (isVec2ALongerThanB(horizontallyClippedRay, verticallyClippedRay))
            {
                clippedRaySegment += verticallyClippedRay;
                isVerticallyClipped = true;
            }
            else if (isVec2ALongerThanB(verticallyClippedRay, horizontallyClippedRay))
            {
                clippedRaySegment += horizontallyClippedRay;
            }
            else
            {
                isClippedOnCorner = true;
            }

            /*
                Draw line representations for the rays on the minimap
            */
            jadel::Vec2 worldRay = rayStartPos + clippedRaySegment;
            if (worldRay.x < 0 || worldRay.y < 0 || worldRay.x >= MAP_WIDTH || worldRay.y >= MAP_HEIGHT)
                break;
            if (seg % 200 == 0 || seg == screenWidth - 1)
            {
                jadel::Color lineColor;
                if (seg == 0 || seg == screenWidth - 1 || seg == screenWidth / 2)
                {
                    lineColor = {1, 1, 0, 0};
                }
                else
                {
                    lineColor = {1, 0, 1, 0};
                }
                jadel::Vec2 rayLineStart = mapStart + rayStartPos * 0.05f;
                jadel::Vec2 rayLineEnd = mapStart + worldRay * 0.05f;

                pushRectRenderable({rayLineEnd.x - 0.005f, rayLineEnd.y - 0.005f, rayLineEnd.x + 0.005f, rayLineEnd.y + 0.005f}, {1, 1, 1, 0});
                pushLineRenderable(rayLineStart, rayLineEnd, lineColor);
                pushLineRenderable(mapStart + screenWorldStart * 0.05, mapStart + screenWorldEnd * 0.05, {1, 1, 0, 1});
                // pushLineRenderable(rayLineStart, rayLineStart + rotatedRayVec * 0.05f, {1, 1, 0, 0});
            }

            float xMargin = 0;
            float yMargin = 0;

            if (isClippedOnCorner)
            {
                xMargin = rayXDir * 0.0001f;
                yMargin = rayYDir * 0.0001f;
            }
            else if (isVerticallyClipped)
            {
                yMargin = rayYDir * 0.0001f;
            }
            else
            {
                xMargin = rayXDir * 0.0001f;
            }

            int content = getSectorContent(worldRay.x + xMargin, worldRay.y + yMargin);
            /*
                If the ray hits a wall, draw it, and move on to the next ray.
            */
            if (content == 1)
            {
                float dist = clippedRaySegment.length();
                if (dist > 0)
                {
                    static float defaultWallScale = 0.8f;
                    static float obliqueWallScale = 0.1f;
                    float cosWallAngle;

                    if (isVerticallyClipped)
                    {
                         
                        cosWallAngle = fabs(getCosBetweenVectors(worldRay - camPos, jadel::Vec2(1.0f, 0)));
                    }
                    else
                    {
                        cosWallAngle = fabs(getCosBetweenVectors(worldRay - camPos, jadel::Vec2(0, 1.0f)));
                    }

                    jadel::Color defaultWallColor = {1, defaultWallScale, defaultWallScale, defaultWallScale};
                    float scale = jadel::lerp(obliqueWallScale, defaultWallScale, 1.0f - cosWallAngle);
                    static float ambientLight = 0;
                    float distanceSquared = (dist * dist) * 0.3;
                    jadel::Color wallColor = {defaultWallColor.a,
                                              jadel::clampf(scale / distanceSquared, 0, scale) + ambientLight,
                                              jadel::clampf(scale / distanceSquared, 0, scale) + ambientLight,
                                              jadel::clampf(scale / distanceSquared, 0, scale) + ambientLight};

                    if (seg == screenWidth / 2)
                    {
                        //wallColor = {1, 1, 0, 0};
                        jadel::message("%f\n", cosWallAngle);

                    }
                    
                    jadel::graphicsDrawRectRelative({currentScreenX, -1.0f / (dist), currentScreenX + columnWidth, 1.0f / (dist)}, wallColor);
                }
                break;
            }
            rayMarchPos = worldRay;
        }
    }

    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            float squareDiameter = 0.05f;
            float xStart = mapStart.x + x * squareDiameter;
            float yStart = mapStart.y + y * squareDiameter;
            jadel::Rectf dim = {xStart, yStart, xStart + squareDiameter, yStart + squareDiameter};
            int content = getSectorContent(x, y);
            static jadel::Color occupiedSectorColor = {0.7f, 0.3f, 0.3f, 0.3f};
            static jadel::Color emptySectorColor = {0.7f, 0.2f, 0.2f, 0.2f};
            jadel::Color sectorColor = content == 0 ? emptySectorColor : occupiedSectorColor;
            // jadel::graphicsDrawRectRelative(dim, sectorColor);
            pushRectRenderable(dim, sectorColor);
        }
    }
    jadel::Vec2 mapDim((float)MAP_WIDTH * 0.05f, (float)MAP_HEIGHT * 0.05f);
    jadel::Vec2 camMapPos = mapStart + jadel::Vec2(camPos.x * 0.05f, camPos.y * 0.05f);
    pushRectRenderable({camMapPos.x - 0.005f, camMapPos.y - 0.005f, camMapPos.x + 0.005f, camMapPos.y + 0.005f}, {0.7f, 1, 0, 0});
    for (int i = 0; i < numRectRenderables; ++i)
    {
        jadel::graphicsDrawRectRelative(rectRenderables[i].rect, rectRenderables[i].color);
    }

    for (int i = 0; i < numLineRenderables; ++i)
    {
        jadel::graphicsDrawLineRelative(lineRenderables[i].start, lineRenderables[i].end, lineRenderables[i].color);
    }
}

void tick()
{

    static float standardSpeed = 1.0f;
    static float runningSpeed = 2.3f;
    static float currentSpeed = standardSpeed;
    static float camRotationSpeed = 140.0f;
    // jadel::message("sc W: %f, sc D: %f\n", screenPlaneWidth, screenPlaneDist);
    if (jadel::inputIsKeyTyped(jadel::KEY_SHIFT))
    {
        currentSpeed = runningSpeed;
    }
    else if (jadel::inputIsKeyReleased(jadel::KEY_SHIFT))
    {
        currentSpeed = standardSpeed;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_X))
    {
        screenPlaneDist += 0.8f * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_Z))
    {
        screenPlaneDist -= 0.8f * frameTime;
    }

    if (jadel::inputIsKeyPressed(jadel::KEY_V))
    {
        screenPlaneWidth += 0.8f * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_C))
    {
        screenPlaneWidth -= 0.8f * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_LEFT))
    {
        camRotation += camRotationSpeed * frameTime;
    }

    if (jadel::inputIsKeyPressed(jadel::KEY_RIGHT))
    {
        camRotation -= camRotationSpeed * frameTime;
    }

    while (camRotation < 0)
        camRotation += 360.0f;
    while (camRotation > 360.0f)
        camRotation -= 360.0f;

    float cRadians = TO_RADIANS(camRotation);
    cameraRotationMatrix = {
        cosf(cRadians), sinf(cRadians), 0,
        -sinf(cRadians), cosf(cRadians), 0,
        0, 0, 1};

    float deg90ToRad = TO_RADIANS(90);

    jadel::Mat3 rotate90DegMatrix = {
        cosf(deg90ToRad), sinf(deg90ToRad), 0,
        -sinf(deg90ToRad), cosf(deg90ToRad), 0,
        0, 0, 1};

    jadel::Vec2 forward = cameraRotationMatrix.mul(jadel::Vec2(0, 1.0f));
    jadel::Vec2 left = rotate90DegMatrix.mul(forward);

    if (jadel::inputIsKeyPressed(jadel::KEY_A))
    {
        camPos += left * currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_D))
    {
        camPos -= left * currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_S))
    {
        camPos -= forward * currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_W))
    {
        camPos += forward * currentSpeed * frameTime;
    }

    if (lastCamPos != camPos)
    {
        //jadel::message("X: %f | Y: %f\n", camPos.x, camPos.y);
    }

    lastCamPos = camPos;

    render();
}

void init()
{
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            int index = x + y * MAP_WIDTH;
            map[index] = y % 2 == 0 && x % 2 == 0 ? 1 : 0;
            jadel::message("%d ", map[index]);
        }
        jadel::message("\n");
    }
}

int JadelMain()
{
    if (!JadelInit(MB(500)))
    {
        jadel::message("Jadel init failed!\n");
        return 0;
    }
    jadel::allocateConsole();
    srand(time(NULL));

    jadel::Window window;
    jadel::windowCreate(&window, "RayCaster", screenWidth, screenHeight);
    jadel::Surface winSurface;
    jadel::graphicsCreateSurface(screenWidth, screenHeight, &winSurface);
    uint32 *winPixels = (uint32 *)winSurface.pixels;

    jadel::graphicsPushTargetSurface(&winSurface);
    jadel::graphicsSetClearColor(0);
    jadel::graphicsClearTargetSurface();

    init();
    Timer frameTimer;
    frameTimer.start();
    uint32 elapsedInMillis = 0;
    uint32 minFrameTime = 1000 / 165;
    while (true)
    {
        JadelUpdate();
        tick();
        jadel::windowUpdate(&window, &winSurface);

        elapsedInMillis = frameTimer.getMillisSinceLastUpdate();
        if (elapsedInMillis < minFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(minFrameTime - elapsedInMillis));
        }
        if (elapsedInMillis > 0)
        {
            frameTime = (float)frameTimer.getMillisSinceLastUpdate() * 0.001f;
        }

        uint32 debugTime = frameTimer.getMillisSinceLastUpdate();
        // jadel::message("%f\n", frameTime);

        frameTimer.update();
        if (jadel::inputIsKeyPressed(jadel::KEY_ESCAPE))
            return 0;
    }
    return 0;
}