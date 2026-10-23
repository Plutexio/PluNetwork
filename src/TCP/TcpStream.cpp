//
// Created by Plutex on 10/23/26.
//

#include "PluNetwork/TCP/TcpStream.h"

#include "PluBase/Log.h"

PluNetwork::TcpStream::TcpStream(Socket socket)
{
    mSocket = std::move(socket);
    PLU_CORE_INFO("New TCP Stream created");
}

Plu::TOwningPointer<PluNetwork::TcpStream> PluNetwork::TcpStream::Connect(Endpoint endpoint)
{
    SocketType socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socketID == INVALID_SOCKET) {
        PLU_CORE_ERROR("Socket creation failed!");
        return nullptr;
    }
    Socket socketObject(socketID);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(endpoint.Port);
    if (inet_pton(AF_INET, endpoint.Name.CStr(), &address.sin_addr) != 1) {
        PLU_CORE_ERROR("Couldn't convert IPv4 address!");
        return nullptr;
    }

    int error = GetLastError();
    if (error != 0) {
        PLU_CORE_ERROR("GetLastError returned {0}", error);
        return nullptr;
    }

    if (connect(socketID, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        PLU_CORE_ERROR("Socket Connection failed! {}", std::strerror(GetLastError()));
        return nullptr;
    }

    return Plu::TOwningPointer(new TcpStream(std::move(socketObject)));
}

PluNetwork::TcpStream::~TcpStream()
= default;

PluNetwork::Endpoint PluNetwork::TcpStream::Peer() const
{
    sockaddr_storage ss{};   // pomieści zarówno IPv4, jak i IPv6
    socklen_t len = sizeof ss;
    if (getpeername(mSocket.GetHandle(), reinterpret_cast<sockaddr*>(&ss), &len) != 0) {
        PLU_CORE_ERROR("getpeername failed");
        return Endpoint{};
    }

    char host[INET6_ADDRSTRLEN] = {};
    std::uint16_t port = 0;

    if (ss.ss_family == AF_INET) {
        const auto& a = reinterpret_cast<const sockaddr_in&>(ss);
        inet_ntop(AF_INET, &a.sin_addr, host, sizeof host);
        port = ntohs(a.sin_port);
    } else if (ss.ss_family == AF_INET6) {
        const auto& a = reinterpret_cast<const sockaddr_in6&>(ss);
        inet_ntop(AF_INET6, &a.sin6_addr, host, sizeof host);
        port = ntohs(a.sin6_port);
    }

    return Endpoint{.Name = host, .Port = port};
}
