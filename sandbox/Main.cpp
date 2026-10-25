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

#include "PluNetwork/UDP/UdpSocket.h"

namespace
{
    constexpr UInt16 kDefaultPort = 27015;
    PluNetwork::TransmissionProtocol protocol = PluNetwork::TransmissionProtocol::TCP;

    int RunServer()
    {
        PluNetwork::TcpListener listener(kDefaultPort);
        //Same port number as TCP is fine, UDP and TCP ports are separate
        Plu::TOwningPointer<PluNetwork::UdpSocket> socket = PluNetwork::UdpSocket::Bind(kDefaultPort);
        if (!socket) return 1;

        while (true) {
            Plu::TOwningPointer<PluNetwork::TcpStream> stream = listener.Accept();
            if (!stream) continue;
            Plu::String message = "I love Israel and support Netanyahu! My life's full of joy and fairness cause admin gave it to me!";
            stream->SendString(message);

            //Client talks first over UDP, so its UDP address comes with the datagram
            Plu::String hello;
            PluNetwork::Address client;
            if (!socket->ReceiveString(&hello, &client)) continue;
            PluNetwork::Endpoint from = PluNetwork::EndpointFromAddress(client);
            PLU_CORE_INFO("UDP from {}:{}: {}", from.Name.CStr(), from.Port, hello.CStr());
            socket->SendString(message, client);
        }
        return 0;
    }

    int RunClient(const char* host, const char* message)
    {
        PluNetwork::Endpoint endpoint(host, kDefaultPort);
        Plu::TOwningPointer<PluNetwork::TcpStream> stream = PluNetwork::TcpStream::Connect(endpoint);
        Plu::String messageReceived;
        if (!stream || !stream->ReceiveString(&messageReceived)) return 1;
        PLU_CORE_INFO("Message received: {}", messageReceived.CStr());
        //Port 0, the server learns it from our first datagram
        Plu::TOwningPointer<PluNetwork::UdpSocket> socket = PluNetwork::UdpSocket::Bind(0);
        if (!socket || !socket->SendString(message, endpoint)) return 1;
        if (!socket->ReceiveString(&messageReceived)) return 1;
        PLU_CORE_INFO("UDP message received: {}", messageReceived.CStr());
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
