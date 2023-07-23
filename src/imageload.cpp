#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "imageload.h"

void initImageLoad()
{
    stbi_set_flip_vertically_on_load(true);
}

bool load_PNG(const char *filename, jadel::Surface *target)
{
    int width;
    int height;
    int channels;
    target->pixels = stbi_load(filename, &width, &height, &channels, 0);
    if (!target->pixels)
        return false;
    for (int i = 0; i < width * height; ++i)
    {
        uint8 *pixel = (uint8 *)target->pixels + (channels * i);
        jadel::flipBytes(pixel, 3);
    }
    target->width = width;
    target->height = height;
    return true;
}
