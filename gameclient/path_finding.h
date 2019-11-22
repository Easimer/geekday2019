#ifndef GAME_CLIENT_PATH_FINDING_H
#define GAME_CLIENT_PATH_FINDING_H

#include "level_mask.h"

struct Path_Node {
    unsigned x, y;
    Path_Node* next;
};

Path_Node* CalculatePath(const Level_Mask* level, int startX, int startY, int destX, int destY);
bool IsPointBehindMe(int myX, int myY, int dirX, int dirY, int pX, int pY);

#endif /* GAME_CLIENT_PATH_FINDING_H */