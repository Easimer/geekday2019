#ifndef GAME_CLIENT_PATH_FINDING_H
#define GAME_CLIENT_PATH_FINDING_H

#include "level_mask.h"

struct Path_Node {
    unsigned x, y;
    Path_Node* next;
};

Path_Node* CalculatePath(const LevelBlocks* level, int startX, int startY, int destX, int destY, bool useDarkNodes = true);
bool IsPointBehindMe(int myX, int myY, int dirX, int dirY, int pX, int pY);
void PathNodeFree(Path_Node* node);
Path_Node* PathNodeFreeSingle(Path_Node* node);

#endif /* GAME_CLIENT_PATH_FINDING_H */
