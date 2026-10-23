//
// Created by Plutex on 10/23/26.
//

#include "PluNetwork/TCP/TcpListener.h"

#include "PluBase/Log.h"

PluNetwork::TcpListener::TcpListener(PortType port) {
    SocketType socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketID == INVALID_SOCKET) {
        PLU_CORE_ERROR("Socket creation failed! {}", strerror(GetLastError()));
        return;
    }
    mSocket = Socket(socketID);
    int opt = 1;
    if (setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) != 0) {
        PLU_CORE_ERROR("Socket Options failed! {}", strerror(GetLastError()));
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(socketID, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        PLU_CORE_ERROR("Socket binding failed! {}", strerror(GetLastError()));
        return;
    }
    if (listen(socketID, SOMAXCONN) != 0) {
        PLU_CORE_ERROR("Socket listening failed! {}", strerror(GetLastError()));
        return;
    }

    PLU_CORE_INFO("Listening for TCP connection on port {}", port);
}

Plu::TOwningPointer<PluNetwork::TcpStream> PluNetwork::TcpListener::Accept() {
    if (!mSocket.IsValid()) {
        PLU_CORE_ERROR("Socket is invalid!");
        return nullptr;
    }
#ifdef PLU_PLATFORM_LINUX
    SocketType clientID = accept4(mSocket.GetHandle(), nullptr, nullptr, SOCK_CLOEXEC);
#else
    SocketType clientID = accept(mSocket.GetHandle(), nullptr, nullptr);
#endif

    if (clientID != INVALID_SOCKET) {
#ifdef PLU_PLATFORM_MACOS
        int on = 1;
        setsockopt(clientID, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
        Socket ownedSocket(clientID);
        return Plu::TOwningPointer(new TcpStream(std::move(ownedSocket)));
    }
    int error = GetLastError();

    if (InterruptedError(error)) {
        PLU_CORE_WARN("Socket accept failed!");
        return nullptr;
    }
    if (ConnectionAbortedError(error)) {
        PLU_CORE_WARN("Connection aborted!");
        return nullptr;
    }
    PLU_CORE_ERROR("Connection failed!");
    return nullptr;
}
