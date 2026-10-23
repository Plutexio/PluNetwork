//
// Created by Plutex on 10/23/26.
//

#include "PluNetwork/TCP/TcpListener.h"

#include "PluBase/Log.h"

PluNetwork::TcpListener::TcpListener(Endpoint endpoint) {
    SocketType socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketID == INVALID_SOCKET) {
        PLU_CORE_ERROR("Socket creation failed");
        return;
    }
    int opt = 1;
    setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_addr.s_addr = htonl(endpoint.Port);

    if (bind(socketID, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        PLU_CORE_ERROR("Socket binding failed!");
        return;
    }
    if (listen(socketID, SOMAXCONN) < 0) {
        PLU_CORE_ERROR("Socket listening failed!");
        return;
    }

    mSocket = Socket(socketID);
    PLU_CORE_INFO("Listening for TCP connection on port {}!", endpoint.Port);
}

PluNetwork::TcpStream PluNetwork::TcpListener::Accept() {
    sockaddr_in client{};
    socklen_t len = sizeof(client);
    SocketType clientID = accept(mSocket.GetHandle(), reinterpret_cast<sockaddr*>(&client), &len);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ip, INET_ADDRSTRLEN);

    Endpoint endpoint{};
    endpoint.Name = ip;

    return TcpStream(endpoint, Socket(clientID));
}
