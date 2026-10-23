//
// Created by Plutex on 10/23/26.
//

#ifndef PLUNETWORK_TCPSTREAM_H
#define PLUNETWORK_TCPSTREAM_H

#include <PluSTL/PluSTL_FWD.h>
#include <PluBase/Base.h>

#include "PluNetwork/Core.h"
#include "PluNetwork/Network.h"
#include "PluNetwork/Socket.h"

namespace PluNetwork
{
    class PLUNETWORK_API TcpStream
    {
        Socket mSocket;
    public:
        static Plu::TOwningPointer<PluNetwork::TcpStream> Connect(Endpoint endpoint);
        ~TcpStream();

        UInt64 Send(DynamicArray<std::byte> data);
        UInt64 Receive(DynamicArray<std::byte>* data);

        //Blocks until all size bytes are sent / received. False on error or closed connection
        bool SendAll(const std::byte* data, UInt64 size);
        bool ReceiveAll(std::byte* data, UInt64 size);

        //Whole buffer with a 4-byte length prefix
        bool SendBuffer(const DynamicArray<std::byte>& data);
        bool ReceiveBuffer(DynamicArray<std::byte>* data, UInt32 maxSize = 16 * 1024 * 1024);

        //String with the same length prefix, without the null terminator
        bool SendString(const Plu::String& string);
        bool ReceiveString(Plu::String* string, UInt32 maxSize = 16 * 1024 * 1024);

        [[nodiscard]] Endpoint Peer() const;
    private:
        explicit TcpStream(Socket socket);
        bool SendFramed(const std::byte* data, UInt64 size);
        friend class TcpListener;
    };
}

#endif //PLUNETWORK_TCPSTREAM_H
