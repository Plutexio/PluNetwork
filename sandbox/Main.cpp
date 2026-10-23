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
#include "PluNetwork/TCP/TcpListener.h"

#include <cstdlib>
#include <cstring>
#include <string_view>

namespace
{
    constexpr UInt16 kDefaultPort = 27015;

    int RunServer()
    {
        PluNetwork::TcpListener listener(kDefaultPort);
        Plu::TOwningPointer<PluNetwork::TcpStream> stream = listener.Accept();
        return 0;
    }

    int RunClient(const char* host, const char* message)
    {
        PluNetwork::Endpoint endpoint(host, kDefaultPort);
        Plu::TOwningPointer<PluNetwork::TcpStream> stream = PluNetwork::TcpStream::Connect(endpoint);
        return 0;
    }

    void PrintUsage()
    {
        PLU_INFO("Usage: PluNetworkSandbox --server");
        PLU_INFO("       PluNetworkSandbox --client [host] [message]");
    }
}

int main(int argc, char** argv)
{
    Plu::Log::Init("NET", "SANDBOX");

    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    PluNetwork::InitializeNetwork();

    int result = 1;
    if (std::strcmp(argv[1], "--server") == 0) {
        result = RunServer();
    } else if (std::strcmp(argv[1], "--client") == 0) {
        const char* host = argc > 2 ? argv[2] : "127.0.0.1";
        const char* message = argc > 3 ? argv[3] : "hello";
        result = RunClient(host, message);
    } else {
        PrintUsage();
    }

    PluNetwork::ShutdownNetwork();
    return result;
}
