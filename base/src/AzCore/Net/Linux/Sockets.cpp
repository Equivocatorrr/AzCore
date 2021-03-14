/*
    File: Linux/Sockets.cpp
    Author: Philip Haynes
*/

#include "../Sockets.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

namespace AzCore {
namespace Net {

struct socketdata {
    i32 sockfd = -1;
    sockaddr_in address;
};

static_assert(sizeof(socketdata) <= 24);

// These only exist because Windows needs them
bool Init() {
    return true;
}
void Deinit() {}

inline void Socket::_Err(const char *explain) {
    error = explain;
    error += strerror(errno);
}

inline socketdata& Socket::Data() {
    return *(socketdata*)dataArr;
}

Socket::Socket() : connected(false), type(TCP), error{} {
    Data() = socketdata();
}

Socket::~Socket() {
    if (connected) Disconnect();
}

bool Socket::Connect(const char *serverAddress, i32 portNumber) {
    hostent *server = gethostbyname(serverAddress);
    socketdata &data = Data();
    if (!server) {
        _Err("Failed to get server: ");
        return false;
    }
    if (type == TCP) {
        // create a new socket in the internet domain (AF_INET) of stream type (SOCK_STREAM), using protocol 0 (default)
        data.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        data.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    // data.sockfd is -1 on failure
    if (data.sockfd < 0) {
        _Err("Failed to create socket: ");
        return false;
    }
    bzero((char*)&data.address, sizeof(data.address));
    data.address.sin_family = AF_INET;
    bcopy((char*)server->h_addr,
          (char*)&data.address.sin_addr.s_addr,
          server->h_length);

    // htons converts the port number from host byte order to network byte order
    data.address.sin_port = htons(portNumber);

    // connect connects the socket to an data.address
    if (connect(data.sockfd, (sockaddr*)&data.address, sizeof(data.address)) < 0) {
        _Err("Failed to connect: ");
        close(data.sockfd);
        return false;
    }
    connected = true;
    return true;
}

bool Socket::Host(i32 portNumber) {
    socketdata &data = Data();
    if (type == TCP) {
        data.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        data.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (data.sockfd < 0) {
        _Err("Failed to create socket: ");
        return false;
    }
    bzero((char*)&data.address, sizeof(data.address));
    data.address.sin_family = AF_INET;
    data.address.sin_addr.s_addr = INADDR_ANY;
    data.address.sin_port = htons(portNumber);
    if (bind(data.sockfd, (sockaddr*)&data.address, sizeof(data.address)) < 0) {
        _Err("Failed to bind: ");
        close(data.sockfd);
        return false;
    }
    // Sets up a connection queue of length 5 (the limit)
    listen(data.sockfd, 5);
    return true;
}

/* Waits for a connection from host. */
bool Socket::Accept(Socket &host) {
    socketdata &data = Data();
    socklen_t addressLength = sizeof(data.address);
    data.sockfd = accept(host.Data().sockfd, (sockaddr*)&data.address, &addressLength);
    if (data.sockfd < 0) {
        _Err("Failed to accept connection: ");
        return false;
    }
    connected = true;
    return true;
}

void Socket::Disconnect() {
    socketdata &data = Data();
    close(data.sockfd);
    data.sockfd = -1;
    connected = false;
}

i32 Socket::Send(char *src, u32 length) {
    socketdata &data = Data();
    i32 numSent = send(data.sockfd, src, length, 0);
    if (numSent < 0) {
        if (errno == ECONNABORTED) {
            Disconnect();
        }
        _Err("Failed to send: ");
    }
    return numSent;
}

i32 Socket::Receive(char *dst, u32 length) {
    socketdata &data = Data();
    i32 numReceived = recv(data.sockfd, dst, length, 0);
    if (numReceived < 0) {
        if (errno == ECONNABORTED) {
            Disconnect();
        }
        _Err("Failed to receive: ");
    }
    return numReceived;
}

} // namespace Net
} // namespace AzCore
