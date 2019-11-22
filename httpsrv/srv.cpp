#include <httpsrv/srv.h>

#define WIN32_LEAN_AND_MEAN
#include "httplib.h"
#include <Windows.h>

struct HTTPServer {
    httplib::Server hs;
    HANDLE hThread;
    int iPort;

    void* pUser;
    HTTPServer_OnBonusCallback pfnOnBonus;
    HTTPServer_OnBonusConsumedCallback pfnOnBonusConsumed;
    HTTPServer_OnGameStartCallback pfnOnGameStart;
};

void HTTPServer_Handler(HTTPServer* pServer, const httplib::Request& req, httplib::Response& res) {
    res.set_content("OK!", "text/plain");
    fprintf(stderr, "REQ: %s\n", req.path.c_str());

    if (req.params.count("track") && req.params.count("you")) {
        // GameInit
        auto& pchTrackID = *req.params.find("track");
        auto& pchPlayerID = *req.params.find("you");

        fprintf(stderr, "GameInit: ID: %s TRACK: %s\n", pchPlayerID.second.c_str(), pchTrackID.second.c_str());

        if (pServer->pfnOnGameStart) {
            pServer->pfnOnGameStart(pServer->pUser, pchPlayerID.second.c_str(), pchTrackID.second.c_str());
        }
    } else if (req.params.count("bonus") && req.params.count("x") && req.params.count("y")) {
        // Bonus
        auto& pchBonus = *req.params.find("bonus");
        auto& pchX = *req.params.find("x");
        auto& pchY = *req.params.find("y");

        int x = std::stoi(pchX.second);
        int y = std::stoi(pchY.second);
        char B = pchBonus.second[0];

        fprintf(stderr, "Bonus: '%c' %d %d\n", B, x, y);

        if (pServer->pfnOnBonus) {
            pServer->pfnOnBonus(pServer->pUser, x, y, pchBonus.second[0]);
        }
    } else {
        fprintf(stderr, "REQ: ??? (%s)\n", req.path.c_str());
    }
}

DWORD WINAPI HTTPServer_ThreadFunc(void* pServer) {
    HTTPServer* pInstance = (HTTPServer*)pServer;

    pInstance->hs.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        HTTPServer_Handler(pInstance, req, res);
    });

    fprintf(stderr, "HTTPServer thread is running and has begun to serve\n");
    if (!pInstance->hs.listen("0.0.0.0", pInstance->iPort)) {
        fprintf(stderr, "LISTEN has failed!\n");
    }

    fprintf(stderr, "LISTEN has ended!\n");

    return 0;
}

HTTPServer* HTTPServer_Create(int port) {
    HTTPServer* ret = NULL;

    if (port > 1024) {
        ret = new HTTPServer;
        ret->iPort = port;
        ret->pUser = NULL;
        ret->pfnOnBonus = NULL;
        ret->pfnOnGameStart = NULL;

        ret->hThread = CreateThread(NULL, 1024, HTTPServer_ThreadFunc, ret, 0, NULL);
    }

    return ret;
}

void HTTPServer_Shutdown(HTTPServer* pServer) {
    if (pServer) {
        pServer->hs.stop();
        WaitForSingleObject(pServer->hThread, 1000);
        delete pServer;
    }
}

#define DEFINE_CALLBACK_SETTER(type) \
void HTTPServer_Set##type##Callback(HTTPServer* hServer, HTTPServer_##type##Callback pfnCallback, void* pUser) { \
    if (hServer) { \
        hServer->pUser = pUser; \
        hServer->pfn##type## = pfnCallback; \
    } \
}

DEFINE_CALLBACK_SETTER(OnBonus);
DEFINE_CALLBACK_SETTER(OnGameStart);
DEFINE_CALLBACK_SETTER(OnBonusConsumed);