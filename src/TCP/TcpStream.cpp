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

    if (connect(socketID, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        PLU_CORE_ERROR("Socket Connection failed! {}", std::strerror(GetLastError()));
        return nullptr;
    }

    return Plu::TOwningPointer(new TcpStream(std::move(socketObject)));
}

PluNetwork::TcpStream::~TcpStream()
{
    PLU_CORE_INFO("TCP Stream destroyed");
}

UInt64 PluNetwork::TcpStream::Send(DynamicArray<std::byte> data)
{
    UInt64 sent = 0;
    while (sent < data.Size()) {
        ssize_t n = send(mSocket.GetHandle(), data.Data() + sent, data.Size() - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (InterruptedError(GetLastError())) continue;
            PLU_CORE_ERROR("Send failed! {}", strerror(GetLastError()));
            break;
        }
        sent += n;
    }
    return sent;
}

UInt64 PluNetwork::TcpStream::Receive(DynamicArray<std::byte> *data)
{
    data->Resize(data->Capacity());
    for (;;) {
        ssize_t n = recv(mSocket.GetHandle(), data->Data(), data->Size(), 0);
        if (n < 0 && InterruptedError(GetLastError())) continue;
        if (n < 0) {
            PLU_CORE_ERROR("Receive failed! {}", strerror(GetLastError()));
            n = 0;
        }
        data->Resize(n);
        return n;
    }
}

bool PluNetwork::TcpStream::SendAll(const std::byte* data, UInt64 size)
{
    UInt64 sent = 0;
    while (sent < size) {
        ssize_t n = send(mSocket.GetHandle(), data + sent, size - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (InterruptedError(GetLastError())) continue;
            PLU_CORE_ERROR("Send failed! {}", strerror(GetLastError()));
            return false;
        }
        sent += n;
    }
    return true;
}

bool PluNetwork::TcpStream::ReceiveAll(std::byte* data, UInt64 size)
{
    UInt64 received = 0;
    while (received < size) {
        ssize_t n = recv(mSocket.GetHandle(), data + received, size - received, 0);
        if (n < 0) {
            if (InterruptedError(GetLastError())) continue;
            PLU_CORE_ERROR("Receive failed! {}", strerror(GetLastError()));
            return false;
        }
        if (n == 0) {
            PLU_CORE_WARN("Connection closed after {} of {} bytes", received, size);
            return false;
        }
        received += n;
    }
    return true;
}

bool PluNetwork::TcpStream::SendFramed(const std::byte* data, UInt64 size)
{
    if (size > UINT32_MAX) {
        PLU_CORE_ERROR("Buffer too large to send! {} bytes", size);
        return false;
    }
    UInt32 header = htonl(static_cast<UInt32>(size));
    return SendAll(reinterpret_cast<const std::byte*>(&header), sizeof(header))
        && SendAll(data, size);
}

bool PluNetwork::TcpStream::SendBuffer(const DynamicArray<std::byte>& data)
{
    return SendFramed(data.Data(), data.Size());
}

bool PluNetwork::TcpStream::ReceiveBuffer(DynamicArray<std::byte>* data, UInt32 maxSize)
{
    UInt32 header = 0;
    if (!ReceiveAll(reinterpret_cast<std::byte*>(&header), sizeof(header))) return false;

    UInt32 size = ntohl(header);
    if (size > maxSize) {
        PLU_CORE_ERROR("Incoming buffer too large! {} bytes (max {})", size, maxSize);
        return false;
    }
    data->Resize(size);
    if (data->Size() != size) {
        PLU_CORE_ERROR("Couldn't allocate {} bytes for incoming buffer!", size);
        return false;
    }
    return ReceiveAll(data->Data(), size);
}

bool PluNetwork::TcpStream::SendString(const Plu::String& string)
{
    return SendFramed(reinterpret_cast<const std::byte*>(string.CStr()), string.Length());
}

bool PluNetwork::TcpStream::ReceiveString(Plu::String* string, UInt32 maxSize)
{
    DynamicArray<std::byte> data;
    if (!ReceiveBuffer(&data, maxSize)) return false;
    *string = Plu::String(reinterpret_cast<const char*>(data.Data()), data.Size());
    return true;
}

PluNetwork::Endpoint PluNetwork::TcpStream::Peer() const
{
    sockaddr_storage ss{};
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
