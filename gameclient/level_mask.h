#ifndef GAMECLIENT_LEVEL_MASK_H
#define GAMECLIENT_LEVEL_MASK_H

struct Level_Mask;

Level_Mask* LevelMaskCreate(const char* pchString);
void LevelMaskFree(Level_Mask* pLevel);

void LevelMaskSize(const Level_Mask*, int* pWidth, int* pHeight);
bool LevelMaskInBounds(const Level_Mask*, int x, int y);

#endif /* GAMECLIENT_LEVEL_MASK_H */
