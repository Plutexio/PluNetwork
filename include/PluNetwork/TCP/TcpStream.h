//
// Created by Plutex on 10/23/26.
//

#ifndef PLUNETWORK_TCPSTREAM_H
#define PLUNETWORK_TCPSTREAM_H

#include "PluNetwork/Core.h"
#include "PluNetwork/Network.h"

namespace PluNetwork
{
    class PLUNETWORK_API TcpStream
    {
    public:
        TcpStream(Endpoint endpoint);
        ~TcpStream();
    };
}

#endif //PLUNETWORK_TCPSTREAM_H
