#include "SocketServer.h"

void SocketServer::SocketThread()
{
    char buffer[1024];

    while( IsActive() )
    {
        int bytes = recv( m_Socket, buffer, sizeof( buffer ), 0 );

        if( bytes > 0 )
        {
            std::string msg( buffer, bytes );
            std::cout << msg;
        }
        else
        {
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
            break;
        }
    }
}

void SocketServer::TryConnect()
{
    Shutdown();

    WSADATA wsa;

    if( WSAStartup( MAKEWORD(2, 2), &wsa ) != 0 )
    {
        std::cout << "WSAStartup failed.\n";
        return;
    }

    m_Socket = socket( AF_INET, SOCK_STREAM, 0 );

    if( !IsActive() )
    {
        std::cout << "Socket creation failed.\n";
        return;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    if( connect( m_Socket, (sockaddr*)&server, sizeof( server ) ) < 0 )
    {
        Shutdown();
        return;
    }

    std::cout << "Connected to CSharp Discord BOT.\n";

    m_Thread = std::thread( &SocketServer::SocketThread, this );
}

void SocketServer::Shutdown()
{
    if( m_Thread.joinable() )
    {
        m_Thread.join();
    }

    if( IsActive() )
    {
        shutdown(m_Socket, SD_BOTH);
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }

    WSACleanup();
}

void SocketServer::Send( const char* message )
{
    if( message == nullptr || message[0] == '\0' )
        return;

    if( !IsActive() )
    {
        TryConnect();
    }

    if( !IsActive() )
        return;

    int result = send( m_Socket, message, static_cast<int>( strlen( message ) ), 0 );

    if( result == SOCKET_ERROR )
    {
        int error = WSAGetLastError();
        std::cerr << "[ERROR] failed sending string: " << error << "\n";
        TryConnect();
    }
}
