//
// Created by Plutex on 10/23/26.
//

#ifndef PLUNETWORK_TCPSTREAM_H
#define PLUNETWORK_TCPSTREAM_H

#include "PluNetwork/Core.h"
#include "PluNetwork/Network.h"
#include "PluNetwork/Socket.h"

namespace PluNetwork
{
    class PLUNETWORK_API TcpStream
    {
        Socket mSocket;
        friend class TcpListener;
    public:
        TcpStream(Endpoint endpoint, Socket socket);
        ~TcpStream();

        Endpoint Peer();
    };
}

#endif //PLUNETWORK_TCPSTREAM_H
