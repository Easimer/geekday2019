#include <string>
#include <mutex>
#include <math.h>

#include <httpsrv/srv.h>
#include <httplib.h>
#include "soap.h"
#include "level_mask.h"
#include "udp.h"
#include "entities.h"
#include "path_finding.h"

#define RAYMARCH_SPEEDOMAT 1
#define EMERGENCY_PATH 1

#define LOOP_FREQ (60)

enum class Game_State {
    Initial,
    EnterTrack,
    InGame,
    Exited,
};

struct Game_Info {
    std::mutex lock;
    Game_State state;

    std::string trackID;
    int playerID;

    GetCheckpointsResponse checkpoints;
    Level_Mask* pLevelMask = NULL;
    LevelBlocks* pLevelBlocks = NULL;
    Path_Node* currentPath = NULL;

    int lastIBX = 0, lastIBY = 0;

    LARGE_INTEGER timeStart, timeEnd;

    GameUdp* pUdp;
};

struct Player_Input {
    float newAngle;
    float newSpeed;
    bool placeMine, shootRocket;
};

#pragma pack(push, 1)
struct Entity_Common_Data {
    uint8_t category;
    uint8_t playerID;
    int8_t speed;
    uint16_t posX;
    uint16_t posY;
};

struct Entity_Car_Data {
    Entity_Common_Data common;

    uint8_t nextCheckpoint;
    uint8_t healthPoints;
    uint8_t numMines;
    uint8_t numRockets;
    uint8_t angleTenth; // angle/10
    uint8_t desiredAngleTenth; // desiredAngle/10
};

#define CAT_CAR (1)
#define CAT_ROCKET (2)
#define CAT_MINE (3)

struct Player_Input_Data {
    uint8_t newSpeed;
    uint8_t newAngle;
    uint8_t inputPlaceMine;
    uint8_t inputShootRocket;
};
#pragma pack(pop)

#ifndef M_PI
#define M_PI (3.1415926)
#endif /* M_PI */

#define RADIANS(deg) ((deg / 180.0) * M_PI)
#define DEGREES(rad) ((rad * 180) / M_PI)
#define SPEED(spd) (spd + 15)

static Game_Info gGameInfo;

static void SendPlayerInput(GameUdp* hUdp, const Player_Input* pInput) {
    Player_Input_Data pkt;

    memset(&pkt, 0, sizeof(pkt));
    pkt.newAngle = pInput->newAngle;
    pkt.newSpeed = SPEED(pInput->newSpeed);
    pkt.inputPlaceMine = pInput->placeMine;
    pkt.inputShootRocket = pInput->shootRocket;

    UDPSend(hUdp, &pkt, sizeof(pkt));
}

static bool AmIClose(int myX, int myY, int tarX, int tarY) {
    return (myX - tarX)* (myX - tarX) + (myY - tarY) * (myY - tarY) <= (20 * 20);
}

static bool FewNodesLeft(Path_Node* node) {
    int N = 0;

    while (node && N < 10) {
        N++;
        node = node->next;
    }

    return N < 10;
}

static Path_Node* CloserNodeHack(int myX, int myY, Path_Node* node) {
    Path_Node* head = node;
    assert(node);
    int mindistsq = (node->x - myX) * (node->x - myX) + (node->y - myY) * (node->y - myY);
    auto min = node;

    node = node->next;

    while (node) {
        auto distsq = (node->x - myX) * (node->x - myX) + (node->y - myY) * (node->y - myY);

        if (distsq < mindistsq) {
            min = node;
            mindistsq = distsq;
        }

        node = node->next;
    }

    while (head != min) {
        auto next = head->next;
        PathNodeFreeSingle(head);
        head = next;
    }

    return head;
}

static bool PointInPath(Path_Node* node, int x, int y) {
    bool ret = false;

    if (node) {
        ret = (node->x == x*20 && node->y == y*20) || PointInPath(node->next, x, y);
    }

    return ret;
}

static void PrintPath() {
    int width, height;
    FILE* f = fopen("maplog.txt", "a+b");
    LevelBlocksSize(gGameInfo.pLevelBlocks, &width, &height);
    fprintf(f, "\n");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (PointInPath(gGameInfo.currentPath, x, y)) {
                fprintf(f, "+");
            } else if (LevelBlocksInBounds(gGameInfo.pLevelBlocks, x, y)) {
                fprintf(f, " ");
            } else {
                fprintf(f, "#");
            }
        }
        fprintf(f, "\n");
    }
    fflush(f);
    fclose(f);
}

static void DownloadLevelMask(const char* pchTrackID) {
    char qpath[128];
    if (gGameInfo.pLevelMask) {
        LevelBlocksFree(gGameInfo.pLevelBlocks);
        LevelMaskFree(gGameInfo.pLevelMask);
    }

    httplib::Client cli("192.168.1.20");
    int res = snprintf(qpath, 128, "/geekday/DRserver.php?track=%s", pchTrackID);
    assert(res < 127);
    auto response = cli.Get(qpath);
    gGameInfo.pLevelMask = LevelMaskCreate(response->body.c_str());
    gGameInfo.pLevelBlocks = LevelBlocksCreate(gGameInfo.pLevelMask);
}

void OnGameStart(void* pUser, const char* pchPlayerID, const char* pchTrackID) {
    std::lock_guard g(gGameInfo.lock);
    UDPInvalidateBuffer(gGameInfo.pUdp);
    gGameInfo.state = Game_State::EnterTrack;
    fprintf(stderr, "GAMESTATE HAS CHANGED TO EnterTrack\n");
    gGameInfo.trackID = pchTrackID;
    gGameInfo.playerID = std::stoi(pchPlayerID);
    gGameInfo.checkpoints = GetCheckpoints(gGameInfo.trackID);
    DownloadLevelMask(pchTrackID);
    // TODO: do HTTP reload only when the track name changes
    gGameInfo.state = Game_State::InGame;
    PathNodeFree(gGameInfo.currentPath);
    gGameInfo.currentPath = NULL;

    gGameInfo.lastIBX = gGameInfo.lastIBY = 0;
    ClearPrimaryEntities();
    ClearSecondaryEntities();
}

void OnBonusEvent(void* pUser, int x, int y, char B) {
    fprintf(stderr, "Bonus event: %d %d %c\n", x, y, B);
    int idx = CreatePickup();
    Entity_Pickup* pPU = GetPickupIdx(idx);

    if (pPU) {
        pPU->common.posX = x;
        pPU->common.posY = y;
        pPU->common.playerID = 0xFF;
        pPU->common.speed = 0;
        switch (B) {
        case 'M':
            pPU->kind = Pickup_Kind::k_PickupMine;
            break;
        case 'H':
            pPU->kind = Pickup_Kind::k_PickupHealth;
            break;
        case 'R':
            pPU->kind = Pickup_Kind::k_PickupRocket;
            break;
        }
    }
}

void OnBonusConsumed(void* pUser, int x, int y, char B) {
    std::lock_guard g(gGameInfo.lock);

    Entity_Pickup* pPU;
    int i = 0;

    while ((pPU = GetPickupIdx(i)) != NULL) {
        if (pPU->common.posX == x && pPU->common.posY == y) {
            // TODO: check kind
            DeletePickupIdx(i);
        }
        i++;
    }
}

#define ENDIANSWAP16(x) ((x>>8) | (x<<8))

void ParseEntities(const uint8_t* pBuf, unsigned cubBuf) {
    unsigned currentOffset = 0;

    ClearPrimaryEntities();

    while (currentOffset < cubBuf) {
        auto com = (Entity_Common_Data*)(pBuf + currentOffset);
        auto car = (Entity_Car_Data*)com;
        unsigned offset = 0;

        com->posX = ENDIANSWAP16(com->posX);
        com->posY = ENDIANSWAP16(com->posY);

        switch (com->category) {
        case CAT_CAR:
        {
            //fprintf(stderr, "Car #%u 0x%x 0x%x\n", car->common.playerID, car->common.posX, car->common.posY);
            offset = sizeof(Entity_Car_Data);

            int idx = CreatePlayer();
            auto p = GetPlayerIdx(idx);
            p->common.playerID = car->common.playerID;
            p->common.posX = car->common.posX;
            p->common.posY = car->common.posY;
            p->common.speed = car->common.speed;
            p->angle = car->angleTenth * 10;
            p->desiredAngle = car->desiredAngleTenth * 10;
            p->nextCheckpointID = car->nextCheckpoint;
            p->healthPoints = car->healthPoints;
            // TODO: deserialize everything
            break;
        }
        case CAT_ROCKET:
        {
            offset = sizeof(Entity_Common_Data);
            break;
        }
        case CAT_MINE:
        {
            offset = sizeof(Entity_Common_Data);
            break;
        }
        // NOTE: pickups are received from an other channel
        }

        assert(offset != 0);
        currentOffset += offset;
    }
}

void OnWorldUpdate(void* pUser, void* pBuf, unsigned cubBuf) {
    //fprintf(stderr, "GameClient: world update %u bytes\n", cubBuf);
    std::lock_guard g(gGameInfo.lock);

    ParseEntities((uint8_t*)pBuf, cubBuf);

}

static float Raymarch(const Level_Mask* pMask, int myX, int myY, int dirX, int dirY) {
    auto len = sqrt(dirX * dirX + dirY * dirY);
    float dX = dirX / len;
    float dY = dirY / len;
    for (float t = 1; t < 1000; t += 1) {
        if (!LevelMaskInBounds(pMask, myX + t * dX, myY + t * dY)) {
            return t;
        }
    }
    return INT_MAX;
}

static void EnforceFrequencyStart() {
    QueryPerformanceCounter(&gGameInfo.timeStart);
}

static void EnforceFrequencyEnd(const int Hz) {
    LARGE_INTEGER freq = {};
    QueryPerformanceCounter(&gGameInfo.timeEnd);
    LONGLONG timeDiff = gGameInfo.timeEnd.QuadPart - gGameInfo.timeStart.QuadPart;
    QueryPerformanceFrequency(&freq);

    auto millisElapsed = (timeDiff * 1000) / freq.QuadPart;
    long long slp = 1000 / Hz - millisElapsed;
    if (slp > 0) {
        Sleep(slp);
    }
}

static const GetCheckpointsResponse::Line& GetNextCheckpoint(Entity_Player* pLocalPlayer, int myX, int myY) {
    auto checkpointID = pLocalPlayer->nextCheckpointID;
    auto& checkpoint = gGameInfo.checkpoints.lines[checkpointID];
    auto midpointX = (checkpoint.x1 + checkpoint.x0) / 2;
    auto midpointY = (checkpoint.y1 + checkpoint.y0) / 2;
    if ((myX - midpointX) * (myX - midpointX) + (myY - midpointY) * (myY - midpointY) <= 20 * 20) {
        fprintf(stderr, "AI: next checkpoint hack\n");
        checkpointID = ((checkpointID + 1) % gGameInfo.checkpoints.lines.size());

        checkpoint = gGameInfo.checkpoints.lines[checkpointID];
        midpointX = (checkpoint.x1 + checkpoint.x0) / 2;
        midpointY = (checkpoint.y1 + checkpoint.y0) / 2;
    }
    return checkpoint;
}

int main(int argc, char** argv) {
    auto hUDP = UDPCreate();
    UDPSetUpdateCallback(hUDP, OnWorldUpdate, NULL);
    UDPListen(hUDP);

    gGameInfo.pUdp = hUDP;

    auto hHTTPSrv = HTTPServer_Create(8000);

    HTTPServer_SetOnGameStartCallback(hHTTPSrv, OnGameStart, &gGameInfo);
    HTTPServer_SetOnBonusCallback(hHTTPSrv, OnBonusEvent, &gGameInfo);
    HTTPServer_SetOnBonusConsumedCallback(hHTTPSrv, OnBonusEvent, &gGameInfo);

    while (gGameInfo.state != Game_State::Exited) {
        EnforceFrequencyStart();
        if (gGameInfo.state == Game_State::InGame) {
            std::lock_guard g(gGameInfo.lock);
            auto pLocalPlayer = GetPlayerByID(gGameInfo.playerID);
            if (!pLocalPlayer) {
                // No world update received yet
                EnforceFrequencyEnd(LOOP_FREQ);
                continue;
            }

            /*
            if (gGameInfo.trackID == "CLEAR") {
                // NOTE: would hang PF
                EnforceFrequencyEnd(LOOP_FREQ);
                continue;
            }
            */

            if (pLocalPlayer->healthPoints <= 0) {
                EnforceFrequencyEnd(LOOP_FREQ);
                continue;
            }

            int tarX, tarY;
            auto myX = pLocalPlayer->common.posX;
            auto myY = pLocalPlayer->common.posY;
            bool shouldRecalcPath = gGameInfo.currentPath == NULL;
            bool bOOB = false;

            if (gGameInfo.currentPath != NULL) {
                tarX = gGameInfo.currentPath->x;
                tarY = gGameInfo.currentPath->y;
                int dirX = cos(RADIANS(pLocalPlayer->angle));
                int dirY = sin(RADIANS(pLocalPlayer->angle));
                //while (gGameInfo.currentPath && IsPointBehindMe(myX, myY, dirX, dirY, tarX, tarY)) {
                while (gGameInfo.currentPath && AmIClose(myX, myY, tarX, tarY)) {
                    gGameInfo.currentPath = PathNodeFreeSingle(gGameInfo.currentPath);
                    if (gGameInfo.currentPath) {
                        tarX = gGameInfo.currentPath->x;
                        tarY = gGameInfo.currentPath->y;
                    }
                    PrintPath();
                }

                if (gGameInfo.currentPath == NULL) {
                    fprintf(stderr, "AI: cached path has been exhausted, requesting recalc\n");
                    shouldRecalcPath = true;
                } else {
                    // Check if we're closer to a future node in the cached path than the first one
                    gGameInfo.currentPath = CloserNodeHack(myX, myY, gGameInfo.currentPath);

                    if (gGameInfo.currentPath && FewNodesLeft(gGameInfo.currentPath)) {
                        // ez kell ide kulonben korozni fog CHK1-nel
                        //fprintf(stderr, "AI:exhausted after few node hack, requesting recalc\n");
                        int pX = gGameInfo.currentPath->x;
                        int pY = gGameInfo.currentPath->y;
                        PathNodeFree(gGameInfo.currentPath);
                        gGameInfo.currentPath = NULL;
                        shouldRecalcPath = false;
                        auto& checkpoint = GetNextCheckpoint(pLocalPlayer, myX, myY);
                        auto midpointX = (checkpoint.x1 + checkpoint.x0) / 2;
                        auto midpointY = (checkpoint.y1 + checkpoint.y0) / 2;

                        gGameInfo.currentPath = CalculatePath(gGameInfo.pLevelBlocks,
                            pX, pY,
                            midpointX, midpointY
                        );
                    }
                    else if (!gGameInfo.currentPath) {
                        fprintf(stderr, "AI: cached path has been exhausted after closed node hack, requesting recalc\n");
                        shouldRecalcPath = true;
                    }
                }
            }

            if (shouldRecalcPath) {
                auto checkpointID = pLocalPlayer->nextCheckpointID;
                //auto& checkpoint = gGameInfo.checkpoints.lines[checkpointID];
                auto& checkpoint = GetNextCheckpoint(pLocalPlayer, myX, myY);
                auto midpointX = (checkpoint.x1 + checkpoint.x0) / 2;
                auto midpointY = (checkpoint.y1 + checkpoint.y0) / 2;

                //int dirX = (midpointX - myX);
                //int dirY = (midpointY - myY);
                //float len = sqrt((dirX * dirX) + (dirY * dirY));
                //int dir2X = (dirX / len) * 32;
                //int dir2Y = (dirY / len) * 32;
                //int vpX = myX + dir2X;
                //int vpY = myY + dir2Y;
                int vpX = midpointX;
                int vpY = midpointY;

                gGameInfo.currentPath = CalculatePath(gGameInfo.pLevelBlocks,
                    pLocalPlayer->common.posX, pLocalPlayer->common.posY,
                    //midpointX, midpointY);
                    vpX, vpY);
                assert((unsigned long long)gGameInfo.currentPath != 0xdddddddddddddddd);
                shouldRecalcPath = false;

                if (gGameInfo.currentPath == NULL) {

                } else {
                    /*
                    fprintf(stderr, "AI: path has been recalculated\n");
                    auto cur = gGameInfo.currentPath;
                    while (cur) {
                        fprintf(stderr, "AI: path(%d, %d)\n", cur->x, cur->y);
                        cur = cur->next;
                    }
                    fprintf(stderr, "AI: path(END)\n");
                    */
                    PrintPath();
                }

            }
            auto checkpointID = pLocalPlayer->nextCheckpointID;

            if (gGameInfo.currentPath) {
                tarX = gGameInfo.currentPath->x;
                tarY = gGameInfo.currentPath->y;
            } else {
                //auto checkpoint = gGameInfo.checkpoints.lines[checkpointID];
                auto& checkpoint = GetNextCheckpoint(pLocalPlayer, myX, myY);
                auto midpointX = (checkpoint.x1 + checkpoint.x0) / 2;
                auto midpointY = (checkpoint.y1 + checkpoint.y0) / 2;
#if EMERGENCY_PATH
                gGameInfo.currentPath = CalculatePath(gGameInfo.pLevelBlocks,
                    myX, myY, midpointX, midpointY, true);
                tarX = gGameInfo.currentPath->x;
                tarY = gGameInfo.currentPath->y;
#else
                tarX = gGameInfo.lastIBX;
                tarY = gGameInfo.lastIBY;
#endif
                fprintf(stderr, "AI: out of bounds\n");
                bOOB = true;
            }

            auto dirX = (int)tarX - (int)myX;
            auto dirY = (int)tarY - (int)myY;

            //auto deg = DEGREES(atan2(-dirY, dirX));
            auto deg = DEGREES(atan2(dirY, dirX));
            float angle = deg / 10;

            if (!bOOB) {
                gGameInfo.lastIBX = myX;
                gGameInfo.lastIBY = myY;
            }

            while (angle < 0) {
                angle += 36;
            }

            while (angle >= 36) {
                angle -= 36;
            }

            float curAngle = pLocalPlayer->angle;

            bool dorifto = abs(pLocalPlayer->angle - pLocalPlayer->desiredAngle) / 180.0f > 0.75f;

            float dist = Raymarch(gGameInfo.pLevelMask, myX, myY, dirX, dirY);
            float newSpeed;

#if RAYMARCH_SPEEDOMAT
            // Turbo
            auto dA = abs(pLocalPlayer->angle - pLocalPlayer->desiredAngle);
            if ((dist > 150 && dA < 5) || (dist > 50 && dA > 45 && dA < 105)) {
                newSpeed = 13;
            } else {

                // Normal
                if (dist > 200 || bOOB) {
                    newSpeed = 7;

                    if (dist > 525) {
                        newSpeed = 12;
                    } else if (dist > 450) {
                        newSpeed = 11;
                    } else if (dist > 300) {
                        newSpeed = 10;
                    }

                } else {
                    newSpeed = 5 + dist / 50.0f;
                }
            }
#endif /* RAYMARCH_SPEEDOMAT */

#if DORIFTO_SPEEDOMAT
            if (dorifto) {
                newSpeed = 8;
            } else {
                newSpeed = 4;
            }
#endif /* DORIFTO_SPEEDOMAT */

            //fprintf(stderr, "AI: TARGET(%d, %d) DIR(%d, %d) ANGLE(%f) CHECKP(#%d) NEWSPD(%f)\n", tarX, tarY, dirX, dirY, angle, checkpointID, newSpeed);
            fprintf(stderr, "SPD: %f\n", newSpeed);

            Player_Input inp;
            inp.newAngle = (int)angle;
            inp.newSpeed = newSpeed;
            inp.placeMine = pLocalPlayer->countMines > 0;
            inp.shootRocket = false;

            // Is there a player in front of us

            if (pLocalPlayer->countRockets > 0) {
                int i = 0;
                Entity_Player* p;
                while ((p = GetPlayerIdx(i)) != NULL) {
                    auto pdx = p->common.posX - pLocalPlayer->common.posX;
                    auto pdy = p->common.posY - pLocalPlayer->common.posY;
                    auto d = sqrt(pdx * pdx + pdy * pdy);
                    if (d < 100) {
                        fprintf(stderr, "DIST: %d %f\n", p->common.playerID, d);
                        auto dlen = sqrt(dirX * dirX + dirY * dirY);
                        auto dot = (pdx / d) * dirX / dlen + (pdy / d) * dirY / dlen;
                        if (dot > 0.5) {
                            fprintf(stderr, "DOT: %d %f\n", p->common.playerID, dot);
                            inp.shootRocket = true;
                        }
                    }
                    i++;
                }
            }

            //inp.newAngle = 9;
            SendPlayerInput(hUDP, &inp);

        }
        EnforceFrequencyEnd(LOOP_FREQ);
    }

    UDPShutdown(hUDP);

    HTTPServer_Shutdown(hHTTPSrv);
    return 0;
}