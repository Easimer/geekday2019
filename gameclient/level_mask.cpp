#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "level_mask.h"
#include <string.h>

struct Level_Mask {
    uint8_t* data;
    int width, height;
};

Level_Mask* LevelMaskCreate(const char* pchString) {
    Level_Mask* ret = NULL;
    int width = 0;
    int height = 0;
    int i = 0;

    if (pchString) {
        ret = new Level_Mask;
        fprintf(stderr, "PARSING LEVEL MASK\n");
        // Determine width
        while (pchString[i] != '\n' && pchString[i]) {
            i++;
            width++;
        }
        fprintf(stderr, "WIDTH: %d\n", width);
        auto lineSiz = width + 1;
        int debugLen = strlen(pchString);

        while (pchString[i]) {
            assert(pchString[i] == '\n');
            height++;
            if (pchString[i + 1]) {
                i += lineSiz;
                assert(i <= debugLen);
            } else {
                // i points to the last newline
                i++;
            }
        }

        fprintf(stderr, "HEIGHT: %d\n", width);

        ret->width = width;
        ret->height = height;

        ret->data = new uint8_t[width * height];
        memset(ret->data, 0, sizeof(uint8_t) * width * height);
        assert(ret->data);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                ret->data[y * width + x] = (pchString[y * lineSiz + x] - '0');
            }
        }
    }

    return ret;
}
void LevelMaskFree(Level_Mask* pLevel) {
    if (pLevel) {
        if (pLevel->data) {
            delete[] pLevel->data;
        }
        delete pLevel;
    }
}

void LevelMaskSize(Level_Mask* pLevel, int* pWidth, int* pHeight) {
    if (pLevel) {
        if (pWidth) *pWidth = pLevel->width;
        if (pHeight) *pHeight = pLevel->height;
    }
}

bool LevelMaskInBounds(const Level_Mask* pLevel, int x, int y) {
    bool ret = false;
    if (pLevel) {
        if (x >= 0 && x < pLevel->width && y >= 0 && y < pLevel->height) {
            ret = pLevel->data[y * pLevel->height + x] == 0x01;
        }
    }
    return false;
}
