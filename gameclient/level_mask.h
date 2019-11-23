#ifndef GAMECLIENT_LEVEL_MASK_H
#define GAMECLIENT_LEVEL_MASK_H

struct Level_Mask;

Level_Mask* LevelMaskCreate(const char* pchString);
void LevelMaskFree(Level_Mask* pLevel);

// w, h: pixel count
void LevelMaskSize(const Level_Mask*, int* pWidth, int* pHeight);
// x, y: pixel index
bool LevelMaskInBounds(const Level_Mask*, int x, int y);

struct LevelBlocks;

LevelBlocks* LevelBlocksCreate(Level_Mask* pLevelMask);
void LevelBlocksFree(LevelBlocks* pLevel);

// w, h: cluster count
void LevelBlocksSize(const LevelBlocks*, int* pWidth, int* pHeight);
// x, y: cluster index
bool LevelBlocksInBounds(const LevelBlocks*, int x, int y);
// x, y: cluster index
unsigned LevelBlockDistanceFromWall(const LevelBlocks*, int x, int y);

#endif /* GAMECLIENT_LEVEL_MASK_H */
