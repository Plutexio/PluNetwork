//
// Created by Plutex on 10/22/26.
//

#ifndef PLUNETWORK_NETWORK_H
#define PLUNETWORK_NETWORK_H

#include <PluBase/Base.h>

#include "Socket.h"
#include "PluSTL/String/String.h"

namespace PluNetwork
{
    void InitializeNetwork();
    void ShutdownNetwork();

    enum class TransmissionProtocol
    {
        TCP = 0,
        UDP,
        NumProtocols
    };

    struct Endpoint
    {
        Plu::String Name;
        PortType Port;
    };
}

#endif //PLUNETWORK_NETWORK_H
