//
// Created by Plutex on 10/24/26.
//

#include "PluNetwork/UDP/UdpSocket.h"

#include "PluBase/Log.h"


Plu::TOwningPointer<PluNetwork::UdpSocket> PluNetwork::UdpSocket::Bind(Endpoint endpoint)
{
    SocketType socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_UDP);

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

    if (connect(socketID, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        PLU_CORE_ERROR("Socket Connection failed! {}", std::strerror(GetLastError()));
        return nullptr;
    }

    return Plu::TOwningPointer(new UdpSocket(std::move(socketObject)));
}

PluNetwork::UdpPacket PluNetwork::UdpSocket::Receive(const std::byte *data, UInt64 size)
{
    for (;;) {
        sockaddr_storage ss{};

#ifdef PLU_PLATFORM_WINDOWS
        int len = sizeof ss;
        int n = recvfrom(fd, reinterpret_cast<char*>(data), size,
                           0, reinterpret_cast<sockaddr*>(&ss), &len);
        if (n == SOCKET_ERROR) {
            const int err = GetLastError();
            if (err == WSAEMSGSIZE)
                return {buf.size(), to_endpoint(ss), true};
            if (err == WSAECONNRESET)
                continue;
        }
        return {static_cast<std::size_t>(n), EndpointFromSockaddr(&ss), false};

#else
        iovec iov{const_cast<std::byte*>(data), size};
        msghdr msg{};
        msg.msg_name    = &ss;
        msg.msg_namelen = sizeof(ss);
        msg.msg_iov     = &iov;
        msg.msg_iovlen  = 1;

        ssize_t n = recvmsg(mSocket.GetHandle(), &msg, 0);
        if (n < 0) {
            if (InterruptedError(GetLastError())) continue;
        }
        return {static_cast<std::size_t>(n), EndpointFromSockaddr(&ss), (msg.msg_flags & MSG_TRUNC) != 0};
#endif
    }
}

PluNetwork::UdpSocket::UdpSocket(Socket socket)
{
    mSocket = std::move(socket);
}
