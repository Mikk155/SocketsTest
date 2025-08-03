#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>

#include <iostream>
#include <thread>
#include <atomic>

#pragma comment( lib, "ws2_32.lib" )

class SocketServer
{
    public:

        void Shutdown();

        void Send( const char* message );

    private:

        SOCKET m_Socket = INVALID_SOCKET;
        std::thread m_Thread;
        std::atomic<bool> m_Running = false;

        void TryConnect();
        void SocketThread();
};

inline SocketServer g_Sockets;
