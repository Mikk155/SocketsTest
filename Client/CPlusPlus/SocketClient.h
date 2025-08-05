/**
 * @file SocketClient.h
 * @author Mikk155
 * @brief Implement a simple Client socket in its own thread to transfer and get information from a server
 * @version 0.1
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025 Mikk155
 *
 *    MIT License
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a copy
 *    of this software and associated documentation files (the "Software"), to deal
 *    in the Software without restriction, including without limitation the rights
 *    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the Software is
 *    furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
*/

#include <string>
#include <iostream>
#include <thread>
#include <functional>
#include <optional>
#include <atomic>
#include <algorithm>

#pragma once

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )
#else
// -🐧
#endif

class SocketClient
{
    using SocketCallback = std::function<void( const std::string& )>;

    protected:

        std::string Address;
        int Port;
        int RecBufferSize;
        std::optional<SocketCallback> RecCallback;

        std::atomic<bool> m_Online{false};

        std::thread m_Thread;

#ifdef _WIN32
        std::atomic<SOCKET> m_Socket = INVALID_SOCKET;
#else
// -🐧
#endif

    public:

        ~SocketClient()
        {
            Shutdown();
        }

        /**
         * @brief Setup socket client
         * 
         * @param _Address 
         * @param _Port 
         * @param _RecBufferSize Buffer max size clamp for incoming sockets
         * @param _RecCallback Callback for incoming sockets
         */
        virtual void Setup( std::string _Address, int _Port, int _RecBufferSize, std::optional<SocketCallback> _RecCallback = std::nullopt )
        {
            Address = _Address;
            Port = _Port;
            RecBufferSize = std::clamp( _RecBufferSize, 1, SO_MAX_MSG_SIZE );
            RecCallback = _RecCallback;

            char buffer[256];
            sprintf( buffer, "Setup at %s:%i with incoming buffer of %i", Address.c_str(), Port, RecBufferSize );
            print( buffer );

            if( !RecCallback.has_value() )
            {
                print( "No external callback method set. this client won't receive messages." );
            }

            TryConnect();
        }

        virtual void CloseThread()
        {
            if( RecCallback.has_value() && m_Thread.joinable() && std::this_thread::get_id() != m_Thread.get_id() )
            {
                try {
                    m_Thread.join();
                } catch (...) {}
                m_Thread = std::thread();
            }
        }

        virtual void Shutdown()
        {
            CloseThread();

#ifdef _WIN32
            if( IsActive() )
            {
                shutdown(m_Socket, SD_BOTH);
                closesocket(m_Socket);
                m_Socket = INVALID_SOCKET;
            }

            if( m_Online )
            {
                WSACleanup();
                m_Online = false;
            }
#else
// -🐧
#endif
        }

        /**
         * @brief Return whatever the client socket is currently active and has connection to the server
         */
        virtual bool IsActive()
        {
#ifdef _WIN32
            return ( m_Socket != INVALID_SOCKET );
#else
// -🐧
#endif
        }

        /**
         * @brief Send a message to the server socket. if we're not connected it will attempt to connect first.
         */
        virtual void Send( const char* message )
        {
            if( message == nullptr || message[0] == '\0' )
                return;

            if( !IsActive() )
            {
                TryConnect();
            }

            if( !IsActive() )
                return;

#ifdef _WIN32
            int result = send( m_Socket, message, static_cast<int>( strlen( message ) ), 0 );

            if( result == SOCKET_ERROR )
            {
                int error = WSAGetLastError();

                char buffer[256];
                sprintf( buffer, "Error sending string: %i for message:", error );
                print( buffer );
                print( message );

                Shutdown();
            }
#else
// -🐧
#endif
        }

        /**
         * @brief Send a message to the server socket. if we're not connected it will attempt to connect first.
         */
        virtual void Send( const std::string& message )
        {
            if( !message.empty() )
            {
                Send( message.c_str() );
            }
        }

        /**
         * @brief Shutdown connection if any and attempt to connect
         */
        virtual void TryConnect()
        {
            Shutdown();

#ifdef _WIN32
            WSADATA wsa;

            if( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
            {
                print( "WSAStartup failed." );
                return;
            }

            m_Online = true;

            m_Socket = socket( AF_INET, SOCK_STREAM, 0 );

            if( !IsActive() )
            {
                print( "Socket creation failed." );
                Shutdown();
                return;
            }

            sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons( Port );
            server.sin_addr.s_addr = inet_addr( Address.c_str() );

            if( int result = connect( m_Socket, (sockaddr*)&server, sizeof( server ) ); result < 0 )
            {
                char buffer[256];
                sprintf( buffer, "Failed to connect to a server: %i", result );
                print( buffer );
                Shutdown();
                return;
            }
#else
// -🐧
#endif
            print( "Connected to a server" );

            if( RecCallback.has_value() )
            {
                CloseThread();
                m_Thread = std::thread( &SocketClient::SocketThread, this );
            }
        }

    protected:

        virtual void SocketThread()
        {
            char buffer[SO_MAX_MSG_SIZE];

            while( IsActive() )
            {
#ifdef _WIN32
                if( int bytes = recv( m_Socket, buffer, RecBufferSize, 0 ); bytes > 0 )
                {
                    std::string msg( buffer, bytes );

                    RecCallback.value()(msg);
                }
                else
                {
                    closesocket(m_Socket);
                    m_Socket = INVALID_SOCKET;
                    break;
                }
#else
// -🐧
#endif
            }
        }

        /**
         * @brief Print a message to console, overide method to print to the game message system
         */
        virtual void print( const char* message )
        {
            std::cout << "[SocketClient] " << message << std::endl;
        }
};
