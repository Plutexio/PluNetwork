//
// Created by Plutex on 10/23/26.
//

#ifndef PLUNETWORK_TCPSTREAM_H
#define PLUNETWORK_TCPSTREAM_H

#include <PluSTL/PluSTL_FWD.h>

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

        [[nodiscard]] Endpoint Peer() const;
    private:
        explicit TcpStream(Socket socket);
        friend class TcpListener;
    };
}

#endif //PLUNETWORK_TCPSTREAM_H
