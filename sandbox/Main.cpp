//
// Created by Plutex on 2026-10-21.
//
// Hands-on playground for PluNetwork: run it twice, once as a server and once as a client.
//
//     PluNetworkSandbox --server [port]
//     PluNetworkSandbox --client [host] [port] [message]
//
// The server echoes whatever a client sends; the client prints the echo and exits.

#include "PluBase/Log.h"
#include "PluNetwork/Network.h"
#include "PluNetwork/Socket.h"
#include "PluNetwork/SystemSockets.h"

#include <cstdlib>
#include <cstring>
#include <string_view>

namespace
{
    constexpr UInt16 kDefaultPort = 27015;

#ifdef MSG_NOSIGNAL
    constexpr int kSendFlags = MSG_NOSIGNAL;
#else
    constexpr int kSendFlags = 0;
#endif

    void LogSocketError(const char* what)
    {
        const Int32 error = Plu::GetLastSocketError();
        PLU_ERROR("{0} failed: {1} ({2})", what, Plu::DescribeSocketError(error).CStr(), error);
    }

    UInt16 ParsePort(const char* text)
    {
        const long port = std::strtol(text, nullptr, 10);
        return port > 0 && port <= 65535 ? static_cast<UInt16>(port) : kDefaultPort;
    }

    int RunServer(UInt16 port)
    {
        const Plu::SocketHandle listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == Plu::kInvalidSocket) {
            LogSocketError("socket");
            return 1;
        }

        // Lets a restarted server bind the port again right away instead of waiting out the
        // previous run's connections in TIME_WAIT.
        const int reuse = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            LogSocketError("bind");
            Plu::CloseSocket(listener);
            return 1;
        }
        if (listen(listener, SOMAXCONN) != 0) {
            LogSocketError("listen");
            Plu::CloseSocket(listener);
            return 1;
        }

        PLU_INFO("Server listening on port {0} (Ctrl+C to stop)", port);

        // One client at a time, on purpose: the simplest thing that shows bytes going both ways.
        while (true) {
            sockaddr_in clientAddress{};
            socklen_t clientAddressLength = sizeof(clientAddress);
            const Plu::SocketHandle client = accept(listener, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
            if (client == Plu::kInvalidSocket) {
                LogSocketError("accept");
                continue;
            }

            char host[INET_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &clientAddress.sin_addr, host, sizeof(host));
            PLU_INFO("Client connected: {0}:{1}", host, ntohs(clientAddress.sin_port));

            char buffer[512];
            while (true) {
                const int received = static_cast<int>(recv(client, buffer, sizeof(buffer), 0));
                if (received < 0) {
                    LogSocketError("recv");
                    break;
                }
                if (received == 0) {
                    break;  // The client closed its side.
                }
                PLU_INFO("Received {0} byte(s): {1}", received, std::string_view(buffer, received));
                if (send(client, buffer, received, kSendFlags) != received) {
                    LogSocketError("send");
                    break;
                }
            }

            PLU_INFO("Client disconnected: {0}:{1}", host, ntohs(clientAddress.sin_port));
            Plu::CloseSocket(client);
        }
    }

    int RunClient(const char* host, UInt16 port, const char* message)
    {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        if (inet_pton(AF_INET, host, &address.sin_addr) != 1) {
            PLU_ERROR("Not an IPv4 address: {0}", host);
            return 1;
        }

        const Plu::SocketHandle server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server == Plu::kInvalidSocket) {
            LogSocketError("socket");
            return 1;
        }
        if (connect(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            LogSocketError("connect");
            Plu::CloseSocket(server);
            return 1;
        }
        PLU_INFO("Connected to {0}:{1}", host, port);

        const int length = static_cast<int>(std::strlen(message));
        if (send(server, message, length, kSendFlags) != length) {
            LogSocketError("send");
            Plu::CloseSocket(server);
            return 1;
        }

        // The echo can arrive in pieces; collect until we have as much as we sent.
        char buffer[512];
        int total = 0;
        while (total < length && total < static_cast<int>(sizeof(buffer))) {
            const int received = static_cast<int>(recv(server, buffer + total, sizeof(buffer) - total, 0));
            if (received <= 0) {
                if (received < 0) {
                    LogSocketError("recv");
                }
                break;
            }
            total += received;
        }
        PLU_INFO("Echo: {0}", std::string_view(buffer, total));

        Plu::CloseSocket(server);
        return total == length ? 0 : 1;
    }

    void PrintUsage()
    {
        PLU_INFO("Usage: PluNetworkSandbox --server [port]");
        PLU_INFO("       PluNetworkSandbox --client [host] [port] [message]");
    }
}

int main(int argc, char** argv)
{
    Plu::Log::Init("NET", "SANDBOX");

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    if (!Plu::InitNetwork()) {
        return 1;
    }

    int result = 1;
    if (std::strcmp(argv[1], "--server") == 0) {
        result = RunServer(argc > 2 ? ParsePort(argv[2]) : kDefaultPort);
    } else if (std::strcmp(argv[1], "--client") == 0) {
        const char* host = argc > 2 ? argv[2] : "127.0.0.1";
        const UInt16 port = argc > 3 ? ParsePort(argv[3]) : kDefaultPort;
        const char* message = argc > 4 ? argv[4] : "hello";
        result = RunClient(host, port, message);
    } else {
        PrintUsage();
    }

    Plu::ShutdownNetwork();
    return result;
}
