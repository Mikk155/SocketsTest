#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

#include <iostream>
#include <thread>

#pragma comment( lib, "ws2_32.lib" )

class SocketServer
{
    public:

        bool IsActive() {
            return ( m_Socket != INVALID_SOCKET );
        }

        void Shutdown();

        void Send( const char* message );

    private:

        SOCKET m_Socket = INVALID_SOCKET;
        std::thread m_Thread;

        void TryConnect();
        void SocketThread();
};

inline SocketServer g_Sockets;
