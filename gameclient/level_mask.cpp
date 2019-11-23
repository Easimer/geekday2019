#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

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
	static const int scale = 20;
	static const int wallThreshold = 60;
	__forceinline LevelBlock* getBlock(int x, int y) { return &data[y * width + x]; }
	__forceinline const LevelBlock* getBlock(int x, int y) const { return &data[y * width + x]; }
	static __forceinline LevelBlock* getBlock(int x, int y, int width, LevelBlock* data) { return &data[y * width + x]; }
	__forceinline void getBlockCenter(int x, int y, int* centerX, int* centerY) { *centerX = x * scale + scale / 2; *centerY = y * scale + scale / 2; }
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
	static __forceinline int clampXStart(int x) { return x < 0 ? 0 : x; }
	static __forceinline int clampXEnd(int x, int width) { return x > width ? width : x; }
	static __forceinline int clampYStart(int y) { return y < 0 ? 0 : y; }
	static __forceinline int clampYEnd(int y, int height) { return y > height ? height : y; }
	static const int wallCheckRange = 20;
	static void distanceFromWallFast(int x, int y, int width, int height, LevelBlock* data) {
		LevelBlock* checkedBlock = getBlock(x, y, width, data);
		if (checkedBlock->obstacleRatio > wallThreshold) {
			checkedBlock->distanceFromWall = 0;
			return;
		}
		int startX = clampXStart(x - wallCheckRange);
		int endX = clampXEnd(x + wallCheckRange, width);
		int startY = clampYStart(y - wallCheckRange);
		int endY = clampYEnd(y + wallCheckRange, height);
		int minDistance = wallCheckRange + 1;
		for (int otherY = startY; otherY < endY; otherY++)
		{
			for (int otherX = startX; otherX < endX; otherX++)
			{
				LevelBlock* otherBlock = getBlock(otherX, otherY, width, data);
				if (otherBlock->obstacleRatio > wallThreshold) {
					int deltaX = x - otherX;
					int deltaY = y - otherY;
					int distance = sqrt(deltaX * deltaX + deltaY * deltaY);
					if (distance < minDistance) minDistance = distance;
				}
			}
		}
		checkedBlock->distanceFromWall = minDistance;
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

bool LevelMaskInBounds(const Level_Mask* pLevel, int x, int y) {
    bool ret = false;
    if (pLevel) {
        if (x >= 0 && x < pLevel->width && y >= 0 && y < pLevel->height) {
            ret = pLevel->data[y * pLevel->width + x] == 0x01;
        }
    }
    return ret;
}

bool LevelBlocksInBounds(const LevelBlocks* pLevel, int x, int y) {
	bool ret = false;
	if (pLevel) {
		if (x >= 0 && x < pLevel->width && y >= 0 && y < pLevel->height) {
			ret = pLevel->getBlock(x, y)->obstacleRatio < LevelBlocks::wallThreshold;
		}
	}
	return ret;
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
	/*for (int i = 0; i < blockCount; i++)
	{
		blocks[i].distanceFromWall = LevelBlocks::wallCheckRange + 1;
	}*/

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			blockStruct->calculateObstacleRatio(x, y);
		}
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			LevelBlocks::distanceFromWallFast(x, y, width, height, blocks);
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