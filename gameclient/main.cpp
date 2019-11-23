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
    Path_Node* currentPath = NULL;
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
#pragma pack(pop)

#ifndef M_PI
#define M_PI (3.1415926)
#endif /* M_PI */

#define RADIANS(deg) ((deg / 180.0) * M_PI)
#define DEGREES(rad) ((rad * 180) / M_PI)

static Game_Info gGameInfo;

static void DownloadLevelMask(const char* pchTrackID) {
    char qpath[128];
    if (gGameInfo.pLevelMask) {
        LevelMaskFree(gGameInfo.pLevelMask);
    }
    httplib::Client cli("192.168.1.20");
    int res = snprintf(qpath, 128, "/geekday/DRserver.php?track=%s", pchTrackID);
    assert(res < 127);
    auto response = cli.Get(qpath);
    gGameInfo.pLevelMask = LevelMaskCreate(response->body.c_str());
}

void OnGameStart(void* pUser, const char* pchPlayerID, const char* pchTrackID) {
    if (gGameInfo.state == Game_State::Initial) {
        gGameInfo.state = Game_State::EnterTrack;
        fprintf(stderr, "GAMESTATE HAS CHANGED TO EnterTrack\n");
        gGameInfo.trackID = pchTrackID;
        gGameInfo.playerID = std::stoi(pchPlayerID);
        gGameInfo.checkpoints = GetCheckpoints(gGameInfo.trackID);
        DownloadLevelMask(pchTrackID);
        // TODO: do HTTP reload only when the track name changes
        gGameInfo.state = Game_State::InGame;
    } else {
        fprintf(stderr, "Game started again even though we're already in EnterTrack/InGame\n");
    }
}

void OnBonusEvent(void* pUser, int x, int y, char B) {
    fprintf(stderr, "Bonus event: %d %d %c\n", x, y, B);
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
            fprintf(stderr, "Car #%u 0x%x 0x%x\n", car->common.playerID, car->common.posX, car->common.posY);
            offset = sizeof(Entity_Car_Data);

            int idx = CreatePlayer();
            auto p = GetPlayerIdx(idx);
            p->common.playerID = car->common.playerID;
            p->common.posX = car->common.posX;
            p->common.posY = car->common.posY;
            p->common.speed = car->common.speed;
            p->angle = car->angleTenth * 10;
            p->nextCheckpointID = car->nextCheckpoint;
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
    fprintf(stderr, "GameClient: world update %u bytes\n", cubBuf);
    std::lock_guard g(gGameInfo.lock);

    ParseEntities((uint8_t*)pBuf, cubBuf);

}

int main(int argc, char** argv) {
    //testPF();
    auto hHTTPSrv = HTTPServer_Create(8000);

    HTTPServer_SetOnGameStartCallback(hHTTPSrv, OnGameStart, &gGameInfo);
    HTTPServer_SetOnBonusCallback(hHTTPSrv, OnBonusEvent, &gGameInfo);
    HTTPServer_SetOnBonusConsumedCallback(hHTTPSrv, OnBonusEvent, &gGameInfo);

    auto hUDP = UDPCreate();
    UDPSetUpdateCallback(hUDP, OnWorldUpdate, NULL);
    UDPListen(hUDP);

    OnGameStart(NULL, "69", "TR1");

    while (gGameInfo.state != Game_State::Exited) {
        std::lock_guard g(gGameInfo.lock);
        if (gGameInfo.state == Game_State::InGame) {
            auto pLocalPlayer = GetPlayerByID(gGameInfo.playerID);
            if (!pLocalPlayer) {
                // No world update received yet
                continue;
            }
            int tarX, tarY;
            auto myX = pLocalPlayer->common.posX;
            auto myY = pLocalPlayer->common.posY;
            bool shouldRecalcPath = gGameInfo.currentPath == NULL;

            if (gGameInfo.currentPath != NULL) {
                tarX = gGameInfo.currentPath->x;
                tarY = gGameInfo.currentPath->y;
                int dirX = cos(RADIANS(pLocalPlayer->angle));
                int dirY = sin(RADIANS(pLocalPlayer->angle));
                while (IsPointBehindMe(myX, myY, dirX, dirY, tarX, tarY)) {
                    gGameInfo.currentPath = PathNodeFreeSingle(gGameInfo.currentPath);
                    tarX = gGameInfo.currentPath->x;
                    tarY = gGameInfo.currentPath->y;
                }

                if (gGameInfo.currentPath == NULL) {
                    fprintf(stderr, "AI: cached path has been exhausted, requesting recalc\n");
                    shouldRecalcPath = true;
                }
            }

            if (shouldRecalcPath) {
                auto checkpointID = pLocalPlayer->nextCheckpointID;
                auto checkpoint = gGameInfo.checkpoints.lines[checkpointID];
                auto midpointX = (checkpoint.x1 + checkpoint.x0) / 2;
                auto midpointY = (checkpoint.y1 + checkpoint.y0) / 2;

                int dirX = (midpointX - myX);
                int dirY = (midpointY - myY);
                float len = sqrt((dirX * dirX) + (dirY * dirY));
                int dir2X = (dirX / len) * 32;
                int dir2Y = (dirY / len) * 32;
                int vpX = myX + dir2X;
                int vpY = myY + dir2Y;

                // TODO: cache
                auto pBlocks = LevelBlocksCreate(gGameInfo.pLevelMask);

                gGameInfo.currentPath = CalculatePath(pBlocks,
                    pLocalPlayer->common.posX, pLocalPlayer->common.posY,
                    //midpointX, midpointY);
                    vpX, vpY);
                shouldRecalcPath = false;
                assert(gGameInfo.currentPath);

                LevelBlocksFree(pBlocks);

                fprintf(stderr, "AI: path has been recalculated\n");
                auto cur = gGameInfo.currentPath;
                while (cur) {
                    fprintf(stderr, "AI: path(%d, %d)\n", cur->x, cur->y);
                    cur = cur->next;
                }
                fprintf(stderr, "AI: path(END)\n");
            }

            tarX = gGameInfo.currentPath->x;
            tarY = gGameInfo.currentPath->y;
            float angle = DEGREES(atan2(tarY, tarX)) / 10;

            fprintf(stderr, "AI: target is (%d, %d)\n", tarX, tarY);
            fprintf(stderr, "AI: i can be you're angle %f\n", angle);

        }
        Sleep(10);
    }

    UDPShutdown(hUDP);

    HTTPServer_Shutdown(hHTTPSrv);
    return 0;
}