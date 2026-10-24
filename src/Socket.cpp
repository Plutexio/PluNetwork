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

int PluNetwork::ResolveAddress(Endpoint endpoint, sockaddr_storage *addr) {
    int result;
    addrinfo* result_list = NULL;
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    result = getaddrinfo(endpoint.Name.CStr(), Plu::String::FromInt(endpoint.Port).CStr(), &hints, &result_list);
    if (result == 0)
    {
        memcpy(addr, result_list->ai_addr, result_list->ai_addrlen);
        freeaddrinfo(result_list);
    }
    return result;
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
