//
// Created by Plutex on 10/22/26.
//

#include "PluNetwork/Network.h"

void PluNetwork::InitializeNetwork()
{
#ifdef PLU_PLATFORM_WINDOWS
    WSADATA d;
    WSAStartup(MAKEWORD(2, 2), &d);
#endif
}

void PluNetwork::ShutdownNetwork()
{
#ifdef PLU_PLATFORM_WINDOWS
    WSACleanup();
#endif
}
