//
// Created by Plutex on 10/22/26.
//
#include "PluNetwork/Socket.h"

#include <cstring>

#include "PluNetwork/Network.h"

#include "PluBase/Log.h"

PluNetwork::Endpoint PluNetwork::EndpointFromSockaddr(const sockaddr_storage *addr)
{
    if (!addr) return {};
    char host[INET6_ADDRSTRLEN] = {};
    std::uint16_t port = 0;

    if (addr->ss_family == AF_INET) {
        const auto& a = reinterpret_cast<const sockaddr_in&>(*addr);
        inet_ntop(AF_INET, &a.sin_addr, host, sizeof host);
        port = ntohs(a.sin_port);
    } else if (addr->ss_family == AF_INET6) {
        const auto& a = reinterpret_cast<const sockaddr_in6&>(*addr);
        inet_ntop(AF_INET6, &a.sin6_addr, host, sizeof host);
        port = ntohs(a.sin6_port);
    }

    return Endpoint{.Name = host, .Port = port};
}

PluNetwork::Endpoint PluNetwork::EndpointFromAddress(const Address& address)
{
    return EndpointFromSockaddr(&address.Storage);
}

bool PluNetwork::ResolveAddress(const Endpoint& endpoint, int socketType, Address* address)
{
    addrinfo* resultList = nullptr;
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = socketType;
    int result = getaddrinfo(endpoint.Name.CStr(), Plu::String::FromInt(endpoint.Port).CStr(), &hints, &resultList);
    if (result != 0) {
        PLU_CORE_ERROR("Couldn't resolve {}:{}! {}", endpoint.Name.CStr(), endpoint.Port, gai_strerror(result));
        return false;
    }
    std::memcpy(&address->Storage, resultList->ai_addr, resultList->ai_addrlen);
    address->Length = static_cast<socklen_t>(resultList->ai_addrlen);
    freeaddrinfo(resultList);
    return true;
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
