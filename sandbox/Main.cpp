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

#include "PluBase/Types.h"
#include "PluNetwork/Packet/GlmSerialization.h"
#include "PluNetwork/Packet/PacketReader.h"
#include "PluNetwork/Packet/PacketWriter.h"
#include "PluNetwork/UDP/UdpSocket.h"

namespace
{
    constexpr UInt16 kDefaultPort = 27015;
    PluNetwork::TransmissionProtocol protocol = PluNetwork::TransmissionProtocol::TCP;

    //What a chat app would send
    struct ChatMessage
    {
        Plu::String Author;
        Plu::String Text;
        UInt64 Timestamp = 0;

        void Serialize(PluNetwork::PacketWriter& writer) const
        {
            writer.Write(Author);
            writer.Write(Text);
            writer.Write(Timestamp);
        }
        bool Deserialize(PluNetwork::PacketReader& reader)
        {
            return reader.Read(&Author) && reader.Read(&Text) && reader.Read(&Timestamp);
        }
    };

    //What an engine would send
    struct PlayerState
    {
        UInt32 Id = 0;
        Vec3 Position{};
        Quaternion Rotation{};
        bool Alive = false;
        DynamicArray<UInt16> Inventory;

        void Serialize(PluNetwork::PacketWriter& writer) const
        {
            writer.Write(Id);
            writer.Write(Position);
            writer.Write(Rotation);
            writer.Write(Alive);
            writer.Write(Inventory);
        }
        bool Deserialize(PluNetwork::PacketReader& reader)
        {
            return reader.Read(&Id) && reader.Read(&Position) && reader.Read(&Rotation)
                && reader.Read(&Alive) && reader.Read(&Inventory);
        }
    };

    template<typename T>
    bool ReadWhole(const DynamicArray<std::byte>& buffer, T* message)
    {
        PluNetwork::PacketReader reader(buffer);
        if (!reader.Read(message) || !reader.IsAtEnd()) {
            PLU_CORE_ERROR("Malformed packet! {} bytes", buffer.Size());
            return false;
        }
        return true;
    }

    int RunServer()
    {
        PluNetwork::TcpListener listener(kDefaultPort);
        //Same port number as TCP is fine, UDP and TCP ports are separate
        Plu::TOwningPointer<PluNetwork::UdpSocket> socket = PluNetwork::UdpSocket::Bind(kDefaultPort);
        if (!socket) return 1;

        while (true) {
            Plu::TOwningPointer<PluNetwork::TcpStream> stream = listener.Accept();
            if (!stream) continue;
            PluNetwork::PacketWriter writer;
            writer.Write(ChatMessage{
                .Author = "Server",
                .Text = "I love Israel and support Netanyahu! My life's full of joy and fairness cause admin gave it to me!",
                .Timestamp = 1,
            });
            stream->SendBuffer(writer.Buffer());

            //Client talks first over UDP, so its UDP address comes with the datagram
            DynamicArray<std::byte> buffer;
            PluNetwork::Address client;
            ChatMessage hello;
            if (!socket->ReceiveBuffer(&buffer, &client) || !ReadWhole(buffer, &hello)) continue;
            PluNetwork::Endpoint from = PluNetwork::EndpointFromAddress(client);
            PLU_CORE_INFO("UDP from {}:{} <{}>: {}", from.Name.CStr(), from.Port, hello.Author.CStr(), hello.Text.CStr());

            writer.Clear();
            writer.Write(PlayerState{
                .Id = 7,
                .Position = {1.5f, -2.0f, 10.25f},
                .Rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f),
                .Alive = true,
                .Inventory = {3, 14, 15},
            });
            socket->SendBuffer(writer.Buffer(), client);
        }
        return 0;
    }

    int RunClient(const char* host, const char* message)
    {
        PluNetwork::Endpoint endpoint(host, kDefaultPort);
        Plu::TOwningPointer<PluNetwork::TcpStream> stream = PluNetwork::TcpStream::Connect(endpoint);
        DynamicArray<std::byte> buffer;
        ChatMessage chat;
        if (!stream || !stream->ReceiveBuffer(&buffer) || !ReadWhole(buffer, &chat)) return 1;
        PLU_CORE_INFO("<{}>: {}", chat.Author.CStr(), chat.Text.CStr());

        //Port 0, the server learns it from our first datagram
        Plu::TOwningPointer<PluNetwork::UdpSocket> socket = PluNetwork::UdpSocket::Bind(0);
        PluNetwork::PacketWriter writer;
        writer.Write(ChatMessage{.Author = "Client", .Text = message, .Timestamp = 2});
        if (!socket || !socket->SendBuffer(writer.Buffer(), endpoint)) return 1;

        PlayerState player;
        if (!socket->ReceiveBuffer(&buffer) || !ReadWhole(buffer, &player)) return 1;
        PLU_CORE_INFO("Player {} at ({}, {}, {}), alive: {}, {} items",
                      player.Id, player.Position.x, player.Position.y, player.Position.z,
                      player.Alive, player.Inventory.Size());
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
