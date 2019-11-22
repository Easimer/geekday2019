#ifndef GAMECLIENT_UDP_H
#define GAMECLIENT_UDP_H

struct GameUdp;

typedef void(*UDP_OnUpdateCallback)(void* pUser, void* pBuf, unsigned cubBufSiz);

GameUdp* UDPCreate();
void UDPShutdown(GameUdp*);
void UDPSend(GameUdp*, const void* pBuf, unsigned cubBufSiz);
void UDPListen(GameUdp*);
void UDPSetUpdateCallback(GameUdp*, UDP_OnUpdateCallback pfnCallback, void* pUser);


#endif /* GAMECLIENT_UDP_H */
