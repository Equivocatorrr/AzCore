/*
	File: Win32/Sockets.cpp
	Author: Philip Haynes
*/

#include "../Sockets.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

namespace AzCore {
namespace Net {

WSADATA wsaData;

bool Init() {
	return (WSAStartup(MAKEWORD(2,2), &wsaData) == 0);
}

void Deinit() {
	WSACleanup();
}

struct socketdata {
	SOCKET sockh = INVALID_SOCKET;
	addrinfo *address;
};

static_assert(sizeof(socketdata) < 24);

inline void Socket::_Err(const char *explain) {
	error = explain;
	AppendToString(error, WSAGetLastError());
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
	socketdata &data = Data();
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	if (type == TCP) {
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
	} else {
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
	}
	String portString = ToString(portNumber);
	i32 errorCode;
	errorCode = getaddrinfo(serverAddress, portString.data, &hints, &data.address);
	if (errorCode != 0) {
		_Err("Failed to getaddrinfo: ");
		return false;
	}
	data.sockh = socket(data.address->ai_family, data.address->ai_socktype, data.address->ai_protocol);
	if (data.sockh == INVALID_SOCKET) {
		_Err("Failed to create socket: ");
		freeaddrinfo(data.address);
		return false;
	}
	errorCode = connect(data.sockh, data.address->ai_addr, (i32)data.address->ai_addrlen);
	freeaddrinfo(data.address);
	if (errorCode == SOCKET_ERROR) {
		_Err("Failed to connect: ");
		closesocket(data.sockh);
		return false;
	}
	connected = true;
	return true;
}

bool Socket::Host(i32 portNumber) {
	socketdata &data = Data();
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE; // passive means it's meant to be used in a bind call
	if (type == TCP) {
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
	} else {
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
	}
	String portString = ToString(portNumber);
	i32 errorCode;
	errorCode = getaddrinfo(NULL, portString.data, &hints, &data.address);
	if (errorCode != 0) {
		_Err("Failed to getaddrinfo: ");
	}
	data.sockh = socket(data.address->ai_family, data.address->ai_socktype, data.address->ai_protocol);
	if (data.sockh == INVALID_SOCKET) {
		_Err("Failed to create socket: ");
		freeaddrinfo(data.address);
		return false;
	}
	errorCode = bind(data.sockh, data.address->ai_addr, (i32)data.address->ai_addrlen);
	freeaddrinfo(data.address);
	if (errorCode == SOCKET_ERROR) {
		_Err("Failed to bind: ");
		closesocket(data.sockh);
		return false;
	}
	// Sets up a connection queue of length SOMAXCONN (the limit)
	errorCode = listen(data.sockh, SOMAXCONN);
	if (errorCode == SOCKET_ERROR) {
		_Err("Failed to listen: ");
		closesocket(data.sockh);
		return false;
	}
	return true;
}

bool Socket::Accept(Socket &host) {
	socketdata &data = Data();
	i32 addressLength = sizeof(addrinfo);
	data.sockh = accept(host.Data().sockh, (sockaddr*)&data.address, &addressLength);
	if (data.sockh == INVALID_SOCKET) {
		_Err("Failed to accept connection: ");
		return false;
	}
	connected = true;
	return true;
}

void Socket::Disconnect() {
	socketdata &data = Data();
	closesocket(data.sockh);
	data.sockh = INVALID_SOCKET;
	connected = false;
}

i32 Socket::Send(char *src, u32 length) {
	socketdata &data = Data();
	i32 numSent = send(data.sockh, src, length, 0);
	if (numSent == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAECONNABORTED) {
			Disconnect();
		}
		_Err("Failed to send: ");
	}
	return numSent;
}

i32 Socket::Receive(char *dst, u32 length) {
	socketdata &data = Data();
	i32 numReceived = recv(data.sockh, dst, length, 0);
	if (numReceived == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAECONNABORTED) {
			Disconnect();
		}
		_Err("Failed to receive: ");
	}
	return numReceived;
}

} // namespace Net
} // namespace AzCore
