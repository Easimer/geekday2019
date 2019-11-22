#ifndef HTTPSRV_SRV_H
#define HTTPSRV_SRV_H

struct HTTPServer;

typedef void(*HTTPServer_OnBonusCallback)(void* pUser, int nX, int nY, char cBonus);
typedef void(*HTTPServer_OnGameStartCallback)(void* pUser, const char* pPlayerID, const char* pTrackID);

HTTPServer* HTTPServer_Create(int port);
void HTTPServer_Shutdown(HTTPServer*);
void HTTPServer_SetOnBonusCallback(HTTPServer* hServer, HTTPServer_OnBonusCallback pfnCallback, void* pUser);
void HTTPServer_SetGameStartCallback(HTTPServer* hServer, HTTPServer_OnGameStartCallback pfnCallback, void* pUser);

#endif /* HTTPSRV_SRV_H */
