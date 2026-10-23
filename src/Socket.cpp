//
// Created by Plutex on 10/22/26.
//
#include "PluNetwork/Socket.h"

#include "PluNetwork/Network.h"

PluNetwork::Endpoint PluNetwork::EndpointFromSockaddr(sockaddr_storage *addr)
{
    if (!addr) return {};
    char host[INET6_ADDRSTRLEN] = {};
    std::uint16_t port = 0;

    if (addr->ss_family == AF_INET) {
        const auto& a = reinterpret_cast<const sockaddr_in&>(addr);
        inet_ntop(AF_INET, &a.sin_addr, host, sizeof host);
        port = ntohs(a.sin_port);
    } else if (addr->ss_family == AF_INET6) {
        const auto& a = reinterpret_cast<const sockaddr_in6&>(addr);
        inet_ntop(AF_INET6, &a.sin6_addr, host, sizeof host);
        port = ntohs(a.sin6_port);
    }

    return Endpoint{.Name = host, .Port = port};
}

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
