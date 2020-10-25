/*
    File: Sockets.hpp
    Author: Philip Haynes
    Cross-platform network sockets
*/

#include "../basictypes.hpp"
#include "../Memory/String.hpp"

namespace AzCore {
namespace Net {

bool Init();
void Deinit();

struct socketdata;

struct Socket {
    bool connected;
    enum Type : u8 {
        TCP,
        UDP
    } type;
    socketdata *data;
    String error;

    inline void _Err(const char *explain);

    Socket();
    ~Socket();

    // Connect to given server on given port as a client.
    bool Connect(const char *serverAddress, i32 portNumber);
    // Host a server on given port.
    bool Host(i32 portNumber);
    // Accept a connection for host socket, becoming the server socket that handles communications for the connected client.
    bool Accept(Socket &host);
    // Close this socket, effectively disconnecting.
    void Disconnect();
    // Send some bytes, returning actual num sent, or -1 on failure.
    i32 Send(char *src, u32 length);
    // Receive some bytes, returning actual num received or -1 on failure.
    i32 Receive(char *dst, u32 length);
};

} // namespace Net
} // namespace AzCore
