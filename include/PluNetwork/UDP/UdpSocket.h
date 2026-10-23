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
        UInt64 Size;
        Endpoint From;
        bool Truncated;
    };

    class PLUNETWORK_API UdpSocket
    {
        Socket mSocket;
    public:
        static Plu::TOwningPointer<UdpSocket> Bind(Endpoint endpoint);

        UdpSocket(UdpSocket &&) noexcept = default;
        UdpSocket& operator=(UdpSocket &&) noexcept = default;

        void SendAll(const std::byte* data, UInt64 size, Endpoint target);
        UdpPacket Receive(const std::byte* data, UInt64 size);
    private:
        explicit UdpSocket(Socket socket);
    };
}

#endif //PLUNETWORK_UDPSOCKET_H
