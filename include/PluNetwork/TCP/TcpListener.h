//
// Created by Plutex on 10/23/26.
//

#ifndef PLUNETWORK_TCPLISTENER_H
#define PLUNETWORK_TCPLISTENER_H

#include "PluNetwork/Core.h"
#include "PluNetwork/Network.h"
#include "PluNetwork/TCP/TcpStream.h"

namespace PluNetwork
{
    class PLUNETWORK_API TcpListener
    {
        Socket mSocket;
    public:
        TcpListener(Endpoint endpoint);

        //Can Block the execution to wait for client
        TcpStream Accept();
    };
}

#endif //PLUNETWORK_TCPLISTENER_H
