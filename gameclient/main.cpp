#include <string>

#include <httpsrv/srv.h>
#include <httplib.h>
#include "soap.h"
#include "level_mask.h"

enum class Game_State {
    Initial,
    EnterTrack,
    InGame,
};

struct Game_Info {
    Game_State state;

    std::string trackID;
    std::string playerID;

    GetCheckpointsResponse checkpoints;
    Level_Mask* pLevelMask = NULL;
};

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
    } else {
        fprintf(stderr, "Game started again even though we're already in EnterTrack/InGame\n");
    }
}

int main(int argc, char** argv) {
    auto hHTTPSrv = HTTPServer_Create(8000);
    HTTPServer_SetOnGameStartCallback(hHTTPSrv, OnGameStart, &gGameInfo);

    while (true);

    HTTPServer_Shutdown(hHTTPSrv);
    return 0;
}