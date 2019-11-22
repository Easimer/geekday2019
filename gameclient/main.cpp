#include <string>

#include <httpsrv/srv.h>
#include "soap.h"

enum class Game_State {
    Initial,
    EnterTrack,
    InGame,
};

struct Game_Info {
    Game_State state;

    std::string trackID;
    std::string playerID;
};

static Game_Info gGameInfo;

void OnGameStart(void* pUser, const char* pchPlayerID, const char* pchTrackID) {
    if (gGameInfo.state == Game_State::Initial) {
        gGameInfo.state = Game_State::EnterTrack;
        fprintf(stderr, "GAMESTATE HAS CHANGED TO EnterTrack\n");
        gGameInfo.trackID = pchTrackID;
        gGameInfo.playerID = pchPlayerID;
        auto checkpoints = GetCheckpoints(gGameInfo.trackID);
    } else {
        fprintf(stderr, "Game started again even though we're already in EnterTrack/InGame\n");
    }
}

int main(int argc, char** argv) {
    auto hHTTPSrv = HTTPServer_Create(8000);
    HTTPServer_SetOnGameStartCallback(hHTTPSrv, OnGameStart, &gGameInfo);

    GetCheckpoints("CLEAR");

    while (true);

    HTTPServer_Shutdown(hHTTPSrv);
    return 0;
}