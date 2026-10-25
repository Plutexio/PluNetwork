//
// Created by Plutex on 10/24/26.
//

#include <utility>

#include "PluNetwork/UDP/UdpSocket.h"

#include "PluBase/Log.h"


Plu::TOwningPointer<PluNetwork::UdpSocket> PluNetwork::UdpSocket::Bind(PortType port)
{
    SocketType socketID = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketID == INVALID_SOCKET) {
        PLU_CORE_ERROR("Socket creation failed! {}", std::strerror(GetLastError()));
        return nullptr;
    }
    Socket socketObject(socketID);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(socketID, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        PLU_CORE_ERROR("Bind failed! {}", std::strerror(GetLastError()));
        return nullptr;
    }

    return Plu::TOwningPointer(new UdpSocket(std::move(socketObject)));
}

bool PluNetwork::UdpSocket::Send(const std::byte* data, UInt64 size, const Address& target)
{
    if (!mSocket.IsValid()) return false;
    if (size > MaxDatagramSize) {
        PLU_CORE_ERROR("Datagram too large to send! {} bytes (max {})", size, MaxDatagramSize);
        return false;
    }
    for (;;) {
#ifdef PLU_PLATFORM_WINDOWS
        int n = sendto(mSocket.GetHandle(), reinterpret_cast<const char*>(data), static_cast<int>(size), 0,
                       reinterpret_cast<const sockaddr*>(&target.Storage), target.Length);
#else
        ssize_t n = sendto(mSocket.GetHandle(), data, size, 0,
                           reinterpret_cast<const sockaddr*>(&target.Storage), target.Length);
#endif
        if (n < 0) {
            if (InterruptedError(GetLastError())) continue;
            PLU_CORE_ERROR("Send failed! {}", std::strerror(GetLastError()));
            return false;
        }
        return static_cast<UInt64>(n) == size;
    }
}

bool PluNetwork::UdpSocket::Send(const std::byte* data, UInt64 size, const Endpoint& target)
{
    Address address;
    return ResolveAddress(target, SOCK_DGRAM, &address) && Send(data, size, address);
}

bool PluNetwork::UdpSocket::Receive(std::byte* data, UInt64 size, UdpPacket* packet)
{
    if (!mSocket.IsValid()) return false;
    for (;;) {
        Address from;

#ifdef PLU_PLATFORM_WINDOWS
        int len = sizeof(from.Storage);
        int n = recvfrom(mSocket.GetHandle(), reinterpret_cast<char*>(data), static_cast<int>(size),
                         0, reinterpret_cast<sockaddr*>(&from.Storage), &len);
        bool truncated = false;
        if (n == SOCKET_ERROR) {
            const int error = GetLastError();
            //The buffer was filled with the start of the datagram
            if (error == WSAEMSGSIZE) {
                n = static_cast<int>(size);
                truncated = true;
            }
            //ICMP "port unreachable" from an earlier send, not a problem with this socket
            else if (error == WSAECONNRESET || InterruptedError(error)) continue;
            else {
                PLU_CORE_ERROR("Receive failed! {}", error);
                return false;
            }
        }
        from.Length = len;
#else
        iovec iov{data, size};
        msghdr msg{};
        msg.msg_name    = &from.Storage;
        msg.msg_namelen = sizeof(from.Storage);
        msg.msg_iov     = &iov;
        msg.msg_iovlen  = 1;

        ssize_t n = recvmsg(mSocket.GetHandle(), &msg, 0);
        if (n < 0) {
            if (InterruptedError(GetLastError())) continue;
            PLU_CORE_ERROR("Receive failed! {}", std::strerror(GetLastError()));
            return false;
        }
        from.Length = msg.msg_namelen;
        bool truncated = (msg.msg_flags & MSG_TRUNC) != 0;
#endif
        *packet = {static_cast<UInt64>(n), from, truncated};
        return true;
    }
}

bool PluNetwork::UdpSocket::SendBuffer(const DynamicArray<std::byte>& data, const Address& target)
{
    return Send(data.Data(), data.Size(), target);
}

bool PluNetwork::UdpSocket::SendBuffer(const DynamicArray<std::byte>& data, const Endpoint& target)
{
    return Send(data.Data(), data.Size(), target);
}

bool PluNetwork::UdpSocket::ReceiveBuffer(DynamicArray<std::byte>* data, Address* from, UInt32 maxSize)
{
    data->Resize(maxSize);
    if (data->Size() != maxSize) {
        PLU_CORE_ERROR("Couldn't allocate {} bytes for incoming datagram!", maxSize);
        return false;
    }

    UdpPacket packet;
    if (!Receive(data->Data(), maxSize, &packet)) return false;
    if (packet.Truncated) {
        PLU_CORE_ERROR("Incoming datagram too large! (max {} bytes)", maxSize);
        return false;
    }
    data->Resize(packet.Size);
    if (from) *from = packet.From;
    return true;
}

bool PluNetwork::UdpSocket::SendString(const Plu::String& string, const Address& target)
{
    return Send(reinterpret_cast<const std::byte*>(string.CStr()), string.Length(), target);
}

bool PluNetwork::UdpSocket::SendString(const Plu::String& string, const Endpoint& target)
{
    return Send(reinterpret_cast<const std::byte*>(string.CStr()), string.Length(), target);
}

bool PluNetwork::UdpSocket::ReceiveString(Plu::String* string, Address* from, UInt32 maxSize)
{
    DynamicArray<std::byte> data;
    if (!ReceiveBuffer(&data, from, maxSize)) return false;
    *string = Plu::String(reinterpret_cast<const char*>(data.Data()), data.Size());
    return true;
}

PluNetwork::UdpSocket::UdpSocket(Socket socket)
    : mSocket(std::move(socket))
{
}
