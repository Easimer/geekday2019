#ifndef HTTPSRV_SRV_H
#define HTTPSRV_SRV_H

struct HTTPServer;

typedef void(*HTTPServer_OnBonusCallback)(void* pUser, int nX, int nY, char cBonus);
typedef void(*HTTPServer_OnBonusConsumedCallback)(void* pUser, int nX, int nY, char cBonus);
typedef void(*HTTPServer_OnGameStartCallback)(void* pUser, const char* pPlayerID, const char* pTrackID);

#define DECLARE_CALLBACK_SETTER(type) \
void HTTPServer_Set##type##Callback(HTTPServer* hServer, HTTPServer_##type##Callback pfnCallback, void* pUser);

HTTPServer* HTTPServer_Create(int port);
void HTTPServer_Shutdown(HTTPServer*);

DECLARE_CALLBACK_SETTER(OnBonus);
DECLARE_CALLBACK_SETTER(OnBonusConsumed);
DECLARE_CALLBACK_SETTER(OnGameStart);

#endif /* HTTPSRV_SRV_H */
