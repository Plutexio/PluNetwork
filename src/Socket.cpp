//
// Created by Plutex on 10/22/26.
//
#include "PluNetwork/Socket.h"

void PluNetwork::Socket::Reset()
{
    if (!IsValid()) return;
#ifdef PLU_PLATFORM_WINDOWS
    closesocket(mHandle);
#else
    close(mHandle);
#endif
    mHandle = INVALID_SOCKET;
}
