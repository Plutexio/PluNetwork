//
// Created by Plutex on 2026-10-21.
//
// Server and client in one process, over 127.0.0.1. The server binds port 0, so the OS picks a
// free port: tests never collide with each other or with anything else running on the machine.

#include "TestFramework.h"

#include "PluNetwork/Network.h"
#include "PluNetwork/Socket.h"
#include "PluNetwork/SystemSockets.h"

#include <cstring>
#include <thread>

namespace
{
    // MSG_NOSIGNAL: a send to a peer that already closed fails with EPIPE instead of killing the
    // process with SIGPIPE. Linux only; Windows has no SIGPIPE to begin with.
#ifdef MSG_NOSIGNAL
    constexpr int kSendFlags = MSG_NOSIGNAL;
#else
    constexpr int kSendFlags = 0;
#endif

    sockaddr_in LoopbackAddress(UInt16 port)
    {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        return address;
    }

    // A listening TCP socket on 127.0.0.1 with an OS-assigned port, returned through outPort.
    Plu::SocketHandle ListenOnLoopback(UInt16& outPort)
    {
        const Plu::SocketHandle listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == Plu::kInvalidSocket) {
            return Plu::kInvalidSocket;
        }

        sockaddr_in address = LoopbackAddress(0);
        socklen_t addressLength = sizeof(address);
        if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0
            || listen(listener, 1) != 0
            || getsockname(listener, reinterpret_cast<sockaddr*>(&address), &addressLength) != 0) {
            Plu::CloseSocket(listener);
            return Plu::kInvalidSocket;
        }

        outPort = ntohs(address.sin_port);
        return listener;
    }

    // Waits until `socket` has something to read (or, for a listener, a connection to accept).
    // Keeps a test from hanging forever on accept/recv when the other side never shows up.
    bool WaitReadable(Plu::SocketHandle socket, int timeoutMs)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(socket, &readSet);
        timeval timeout{};
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_usec = (timeoutMs % 1000) * 1000;
        // The first argument is ignored by Winsock.
        return select(static_cast<int>(socket + 1), &readSet, nullptr, nullptr, &timeout) > 0;
    }

    // send() may take only part of the buffer; loop until all of it is out.
    bool SendAll(Plu::SocketHandle socket, const char* data, int size)
    {
        while (size > 0) {
            const int sent = static_cast<int>(send(socket, data, size, kSendFlags));
            if (sent <= 0) {
                return false;
            }
            data += sent;
            size -= sent;
        }
        return true;
    }

    // TCP is a byte stream: one send can arrive as several recvs. Loop until `size` bytes came in.
    bool ReceiveExact(Plu::SocketHandle socket, char* data, int size)
    {
        while (size > 0) {
            const int received = static_cast<int>(recv(socket, data, size, 0));
            if (received <= 0) {
                return false;
            }
            data += received;
            size -= received;
        }
        return true;
    }
}

PLU_TEST(Network_InitAndShutdownNest)
{
    PLU_CHECK(Plu::InitNetwork());
    PLU_CHECK(Plu::InitNetwork());
    Plu::ShutdownNetwork();
    Plu::ShutdownNetwork();
}

PLU_TEST(Socket_CloseInvalidIsNoOp)
{
    Plu::CloseSocket(Plu::kInvalidSocket);
    PLU_CHECK(true);
}

PLU_TEST(Socket_DescribeErrorIsNotEmpty)
{
    // ECONNREFUSED / WSAECONNREFUSED: any real error code will do.
#ifdef PLU_PLATFORM_WINDOWS
    const Int32 error = WSAECONNREFUSED;
#else
    const Int32 error = ECONNREFUSED;
#endif
    PLU_CHECK_FALSE(Plu::DescribeSocketError(error).IsEmpty());
}

PLU_TEST(Socket_TcpLoopbackEcho)
{
    PLU_CHECK(Plu::InitNetwork());

    UInt16 port = 0;
    const Plu::SocketHandle listener = ListenOnLoopback(port);
    PLU_CHECK(listener != Plu::kInvalidSocket);
    PLU_CHECK_NE(port, UInt16(0));
    if (listener == Plu::kInvalidSocket) {
        Plu::ShutdownNetwork();
        return;
    }

    constexpr char kMessage[] = "hello";
    constexpr int kMessageSize = sizeof(kMessage) - 1;

    // Server thread: accept one client and send back exactly what it got.
    bool serverEchoed = false;
    std::thread server([&] {
        if (!WaitReadable(listener, 5000)) {
            return;
        }
        const Plu::SocketHandle client = accept(listener, nullptr, nullptr);
        if (client == Plu::kInvalidSocket) {
            return;
        }
        char buffer[kMessageSize] = {};
        serverEchoed = ReceiveExact(client, buffer, kMessageSize) && SendAll(client, buffer, kMessageSize);
        Plu::CloseSocket(client);
    });

    const Plu::SocketHandle client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    const sockaddr_in address = LoopbackAddress(port);
    const bool connected = client != Plu::kInvalidSocket
        && connect(client, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
    PLU_CHECK(connected);

    char reply[kMessageSize] = {};
    if (connected) {
        PLU_CHECK(SendAll(client, kMessage, kMessageSize));
        PLU_CHECK(ReceiveExact(client, reply, kMessageSize));
    }
    Plu::CloseSocket(client);

    server.join();
    Plu::CloseSocket(listener);
    Plu::ShutdownNetwork();

    PLU_CHECK(serverEchoed);
    PLU_CHECK(std::memcmp(reply, kMessage, kMessageSize) == 0);
}
