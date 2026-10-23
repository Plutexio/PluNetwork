//
// Created by Plutex on 10/22/26.
//

#ifndef PLUNETWORK_SOCKET_H
#define PLUNETWORK_SOCKET_H

#include <thread>
#include <utility>
#include <PluBase/Base.h>
#include "Core.h"

#ifdef PLU_PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace PluNetwork
{
    struct Endpoint;
#ifdef PLU_PLATFORM_WINDOWS
    using SocketType = SOCKET;
    #error "Define PortType!"
    #error "Define GetLastError!"
    #error "Define InterruptedError!"
    #error "Define ConnectionAbortedError!"
#else
    using SocketType = int;
    constexpr SocketType INVALID_SOCKET = -1;
    using PortType = in_port_t;
    inline int GetLastError() { return errno; }
    inline bool InterruptedError(int errorCode) { return errorCode == EINTR; }
    inline bool ConnectionAbortedError(int errorCode) { return errorCode == ECONNABORTED; }
#endif

    PLUNETWORK_API Endpoint EndpointFromSockaddr(sockaddr_storage* addr);

    class Socket {
    public:
        Socket() = default;
        explicit Socket(SocketType h) : mHandle(h) {}
        ~Socket() { Reset(); }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
        Socket(Socket&& o) noexcept : mHandle(std::exchange(o.mHandle, INVALID_SOCKET)) {}
        Socket& operator=(Socket&& o) noexcept {
            if (this != &o) { Reset(); mHandle = std::exchange(o.mHandle, INVALID_SOCKET); }
            return *this;
        }

        [[nodiscard]] bool IsValid() const { return mHandle != INVALID_SOCKET; }
        [[nodiscard]] SocketType GetHandle() const { return mHandle; }

    private:
        void Reset();
        SocketType mHandle = INVALID_SOCKET;
    };
}

#endif //PLUNETWORK_SOCKET_H
