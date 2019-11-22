#include <string>

#include <httpsrv/srv.h>
#include <httplib.h>
#include "soap.h"
#include "level_mask.h"
#include "udp.h"

enum class Game_State {
    Initial,
    EnterTrack,
    InGame,
    Exited,
};

struct Game_Info {
    Game_State state;

    std::string trackID;
    std::string playerID;

    GetCheckpointsResponse checkpoints;
    Level_Mask* pLevelMask = NULL;
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
        gGameInfo.playerID = pchPlayerID;
        gGameInfo.checkpoints = GetCheckpoints(gGameInfo.trackID);
        DownloadLevelMask(pchTrackID);
        // TODO: do HTTP reload only when the track name changes
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

    ParseEntities((uint8_t*)pBuf, cubBuf);

}

int main(int argc, char** argv) {
    auto hHTTPSrv = HTTPServer_Create(8000);

    HTTPServer_SetOnGameStartCallback(hHTTPSrv, OnGameStart, &gGameInfo);
    HTTPServer_SetOnBonusCallback(hHTTPSrv, OnBonusEvent, &gGameInfo);
    HTTPServer_SetOnBonusConsumedCallback(hHTTPSrv, OnBonusEvent, &gGameInfo);

    auto hUDP = UDPCreate();
    UDPSetUpdateCallback(hUDP, OnWorldUpdate, NULL);
    UDPListen(hUDP);

    while (gGameInfo.state != Game_State::Exited) {
        if (gGameInfo.state == Game_State::InGame) {

        }
    }

    UDPShutdown(hUDP);

    HTTPServer_Shutdown(hHTTPSrv);
    return 0;
}