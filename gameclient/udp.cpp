#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "udp.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define REMOTE_ADDR "192.168.1.20"
#define UDP_PORT_LISTENER (666)
#define UDP_PORT_REMOTE (9999)

struct GameUdp {
    bool bShutdown;

    SOCKET sockListener, sockRemote;
    struct sockaddr_in addrRemote;

    UDP_OnUpdateCallback pfnOnUpdate;
    void* pUser;

    HANDLE hListenThread;

    int cubBuf;
    uint8_t buf[16];
    int watchdogCounter = 0;
};

#pragma pack(push, 1)
struct Server_Header {
    uint8_t seq;
    uint8_t unused[3];
    uint8_t seq_count;
    uint8_t seq_offset;
};
#pragma pack(pop)

static_assert(sizeof(Server_Header) == 6);

static WSADATA wsa;

GameUdp* UDPCreate() {
    GameUdp* ret = new GameUdp;

    if (ret) {
        sockaddr_in addrListener;
        addrListener.sin_addr.s_addr = ADDR_ANY;
        addrListener.sin_family = AF_INET;
        addrListener.sin_port = htons(UDP_PORT_LISTENER);
        WSAStartup(MAKEWORD(2, 2), &wsa);
        ret->sockListener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(ret->sockListener, (sockaddr*)&addrListener, sizeof(addrListener));

        ret->sockRemote = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        inet_pton(AF_INET, REMOTE_ADDR, &ret->addrRemote.sin_addr.s_addr);
        ret->addrRemote.sin_family = AF_INET;
        ret->addrRemote.sin_port = htons(UDP_PORT_REMOTE);

        ret->pfnOnUpdate = NULL;

        ret->bShutdown = false;
        ret->hListenThread = INVALID_HANDLE_VALUE;
    }

    return ret;
}

void UDPShutdown(GameUdp* p) {
    if (p) {
        TerminateThread(p->hListenThread, 0); // TODO: bruh
        closesocket(p->sockListener);
        closesocket(p->sockRemote);
        delete p;
    }
}

void UDPSend(GameUdp* pUdp, const void* pBuf, unsigned cubBufSiz) {
    bool flush = pUdp->cubBuf != cubBufSiz || pUdp->cubBuf == 0 || (++pUdp->watchdogCounter) > 16;
    if (!flush) {
        flush = (memcmp(pBuf, pUdp->buf, cubBufSiz) != 0);
    }

    if (pUdp->watchdogCounter > 16) {
        pUdp->watchdogCounter = 0;
    }

    if (flush) {
        sendto(pUdp->sockRemote, (char*)pBuf, cubBufSiz, 0, (sockaddr*)&pUdp->addrRemote, sizeof(pUdp->addrRemote));
        memcpy(pUdp->buf, pBuf, cubBufSiz);
        pUdp->cubBuf = cubBufSiz;
    }
}

void UDPInvalidateBuffer(GameUdp* pUdp) {
    pUdp->cubBuf = 0;
}

#define MAX_PAYLOAD_DATA_LEN (1300)
#define MAX_PAYLOAD_LEN (MAX_PAYLOAD_DATA_LEN + sizeof(Server_Header))
#define MAX_SEQUENCE_SIZE (256 * 1300)

static DWORD WINAPI UDPThreadFunc(void* pUdpR) {
    auto pUdp = (GameUdp*)pUdpR;

    uint8_t bufRecv[2048];
    auto pHdr = (Server_Header*)bufRecv;
    auto pDat = (char*)(pHdr + 1);

    sockaddr_in addrFrom;
    int addrFromLen;

    bool recvSequence = false; // currently receiving sequence
    auto bufSequence = (uint8_t*)malloc(MAX_SEQUENCE_SIZE);
    unsigned currentSequence = 0;
    unsigned remainSequence = 0;
    unsigned lenSequence = 0;
    int lenSegment;

    assert(bufSequence);

    fprintf(stderr, "UDP: thread OK\n");

    while (!pUdp->bShutdown) {
        addrFromLen = sizeof(addrFrom);
        lenSegment = recvfrom(pUdp->sockListener, (char*)bufRecv, 2048, 0, (sockaddr*)&addrFrom, &addrFromLen);
        if (lenSegment != SOCKET_ERROR) {
            if (recvSequence) {
                //fprintf(stderr, "UDP: rebuild of sequence 0x%x (N=%d)\n", pHdr->seq, pHdr->seq_count);
                auto offSegment = pHdr->seq_offset * MAX_PAYLOAD_DATA_LEN;
                memcpy(bufSequence + offSegment, pDat, lenSegment - sizeof(Server_Header));
                remainSequence--;
            } else {
                //fprintf(stderr, "UDP: beginning rebuild of new sequence 0x%x (N=%d)\n", pHdr->seq, pHdr->seq_count);
                memcpy(bufSequence, pDat, lenSegment - sizeof(Server_Header));
                lenSequence += lenSegment - sizeof(Server_Header);
                remainSequence = pHdr->seq_count - 1;
                recvSequence = true;
            }

            if (recvSequence && !remainSequence) {
                //fprintf(stderr, "UDP: completed sequence 0x%x\n", currentSequence);
                if (pUdp->pfnOnUpdate) {
                    pUdp->pfnOnUpdate(pUdp->pUser, bufSequence, lenSequence);
                }

                lenSequence = 0;
                currentSequence = 0;
                recvSequence = false;
            }
        } else {
            fprintf(stderr, "UDP: Socket error!\n");
        }
    }

    free(bufSequence);

    fprintf(stderr, "UDP: thread exit\n");

    return 0;
}

void UDPListen(GameUdp* pUdp) {
    assert(pUdp);
    assert(pUdp->hListenThread == INVALID_HANDLE_VALUE);

    pUdp->hListenThread = CreateThread(NULL, 64 * 1024, UDPThreadFunc, pUdp, 0, NULL);
}

void UDPSetUpdateCallback(GameUdp* pUdp, UDP_OnUpdateCallback pfnCallback, void* pUser) {
    pUdp->pfnOnUpdate = pfnCallback;
    pUdp->pUser = pUser;
}
