//
// Created by Plutex on 10/24/26.
//

#ifndef PLUNETWORK_UDPSOCKET_H
#define PLUNETWORK_UDPSOCKET_H

#include "PluNetwork/Core.h"
#include "PluNetwork/Network.h"

#include <PluSTL/PluSTL_FWD.h>

namespace PluNetwork
{
    struct UdpPacket
    {
        UInt64 Size = 0;
        //Sender, can be passed straight back to Send to reply
        Address From;
        //Datagram was bigger than the buffer, the rest of it is lost
        bool Truncated = false;
    };

    class PLUNETWORK_API UdpSocket
    {
        Socket mSocket;
    public:
        //Port 0 lets the OS pick a free one (for clients)
        static Plu::TOwningPointer<UdpSocket> Bind(PortType port);

        UdpSocket(UdpSocket &&) noexcept = default;
        UdpSocket& operator=(UdpSocket &&) noexcept = default;

        //One datagram, sent whole or not at all. False on error
        bool Send(const std::byte* data, UInt64 size, const Address& target);
        //Resolves target on every call, prefer the Address overload for repeated sends
        bool Send(const std::byte* data, UInt64 size, const Endpoint& target);

        //Blocks until one datagram arrives. False on error
        bool Receive(std::byte* data, UInt64 size, UdpPacket* packet);

        //Whole buffer as one datagram, no length prefix needed since UDP keeps message boundaries
        bool SendBuffer(const DynamicArray<std::byte>& data, const Address& target);
        bool SendBuffer(const DynamicArray<std::byte>& data, const Endpoint& target);
        //False on error or when the datagram didn't fit in maxSize. from can be null
        bool ReceiveBuffer(DynamicArray<std::byte>* data, Address* from = nullptr, UInt32 maxSize = MaxDatagramSize);

        //String as one datagram, without the null terminator
        bool SendString(const Plu::String& string, const Address& target);
        bool SendString(const Plu::String& string, const Endpoint& target);
        bool ReceiveString(Plu::String* string, Address* from = nullptr, UInt32 maxSize = MaxDatagramSize);

        //Biggest UDP payload over IPv4 (65535 - 8 byte UDP header - 20 byte IP header)
        static constexpr UInt32 MaxDatagramSize = 65507;
    private:
        explicit UdpSocket(Socket socket);
    };
}

#endif //PLUNETWORK_UDPSOCKET_H
