#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "level_mask.h"
#include <string.h>

typedef uint8_t BYTE;

struct Level_Mask {
    uint8_t* data;
    int width, height;
	__forceinline bool isObstacle(int x, int y) { return data[y * width + x] == 0x00; }
	static __forceinline bool isObstacle(int x, int y, int width, uint8_t* data) { return data[y * width + x] == 0x00; }
};

struct LevelBlock
{
	BYTE obstacleRatio;
	BYTE distanceFromWall;
};

struct LevelBlocks {
	LevelBlock* data;
	int width, height;
	Level_Mask* mask;
	const int scale = 20;
	__forceinline LevelBlock* getBlock(int x, int y) { return &data[y * width + x]; }
	static __forceinline LevelBlock* getBlock(int x, int y, int width, LevelBlock* data) { return &data[y * width + x]; }
	__forceinline void getBlockCenter(int x, int y, int* centerX, int* centerY) { centerX = x * scale + scale / 2; centerY = y * scale + scale / 2; }
	void calculateObstacleRatio(int x, int y) {
		int startX = x * scale;
		int endX = x + scale;
		int startY = y * scale;
		int endY = startY + scale;
		int obstacleCount = 0;
		BYTE* maskData = mask->data;
		int maskWidth = mask->width;
		for (int y = startY; y < endY; y++)
		{
			for (int x = startX; x < endX; x++)
			{
				if (Level_Mask::isObstacle(x, y, maskWidth, maskData))
					obstacleCount++;
			}
		}
		getBlock(x, y)->obstacleRatio = obstacleCount * 0xFF / (scale * scale);
	}
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

bool LevelMaskInBounds(Level_Mask* pLevel, int x, int y) {
    bool ret = false;
    if (pLevel) {
        if (x >= 0 && x < pLevel->width && y >= 0 && y < pLevel->height) {
            ret = pLevel->data[y * pLevel->height + x] == 0x01;
        }
    }
    return false;
}


LevelBlocks* LevelBlocksCreate(Level_Mask* mask) {
	int width = mask->width / LevelBlocks::scale;
	int height = mask->height / LevelBlocks::scale;
	int blockCount = width * height;
	LevelBlock* blocks = new LevelBlock[blockCount];
	LevelBlocks* blockStruct = new LevelBlocks();
	blockStruct->data = blocks;
	blockStruct->mask = mask;
	blockStruct->width = width;
	blockStruct->height = height;
	for (int i = 0; i < blockCount; i++)
	{
		blocks[i]->distanceFromWall = 0xFF;
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			blockStruct->calculateObstacleRatio(x, y);
		}
	}
	return blockStruct;
}

void LevelBlocksFree(LevelBlocks* levelBlocks) {
	if (levelBlocks) {
		if (levelBlocks->data) {
			delete[] levelBlocks->data;
		}
		delete levelBlocks;
	}
}