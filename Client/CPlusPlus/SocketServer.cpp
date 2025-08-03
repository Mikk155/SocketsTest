#include "SocketServer.h"

void SocketServer::SocketThread()
{
    char buffer[1024];
    while( m_Running )
    {
        int bytes = recv( m_Socket, buffer, sizeof( buffer ), 0 );

        if( bytes > 0 )
        {
            std::string msg( buffer, bytes );
            std::cout << msg;
        }
        else
        {
            // Error o conexión cerrada
            m_Running = false;
            break;
        }
    }
}

void SocketServer::TryConnect()
{
    if( m_Running )
        return;

    WSADATA wsa;

    if( WSAStartup( MAKEWORD(2, 2), &wsa ) != 0 )
    {
        std::cout << "WSAStartup failed.\n";
        return;
    }

    m_Socket = socket( AF_INET, SOCK_STREAM, 0 );

    if( m_Socket == INVALID_SOCKET )
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
        return;
    }

    std::cout << "Connected to CSharp Discord BOT.\n";

    m_Running = true;
    m_Thread = std::thread( &SocketServer::SocketThread, this );
}

void SocketServer::Shutdown()
{
    if( m_Running )
    {
        shutdown( m_Socket, SD_BOTH );

        closesocket( m_Socket );

        if( m_Thread.joinable() )
        {
            m_Thread.join();
        }

        WSACleanup();

        m_Running = false;
    }
}

void SocketServer::Send( const char* message )
{
    if( message == nullptr || message[0] == '\0' )
        return;

    if( m_Socket == INVALID_SOCKET )
    {
        TryConnect();
    }

    int result = send( m_Socket, message, static_cast<int>( strlen( message ) ), 0 );

    if( result == SOCKET_ERROR )
    {
        int error = WSAGetLastError();
        std::cerr << "[ERROR] failed sending string: " << error << "\n";

        if( error == WSAECONNRESET || error == WSAENOTCONN || error == WSAECONNABORTED )
        {
            Shutdown();
            TryConnect();
        }
    }
}
