/**
 *    MIT License
 *
 *    Copyright (c) 2025 Mikk155
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

#include "SocketClient.h"

#include <csignal>

// Simulate a game server running syncronously
std::atomic<bool> g_GameServerRunning;
std::mutex _STOP_GAME;

inline SocketClient g_Sockets;

void MyMessageHandler( const std::string& message )
{
    std::cout << message << std::endl;;
}

int main()
{
    g_Sockets.Setup( "127.0.0.1", 5000, 128, MyMessageHandler );

    // Have a thread for testing a simulation of game messages
    std::thread WaitForUserInputToExit( []()
    {
        std::cout << "Type 'quit' and press Enter to exit the program at any moment.\n";

        std::string line;

        while( std::getline( std::cin, line ) )
        {
            if( line == "quit" )
            {
                std::lock_guard<std::mutex> lock(_STOP_GAME);
                g_GameServerRunning = false;
                break;
            }
            else
            {
                g_Sockets.Send( line.c_str() );
            }
        }
    } );

    auto ExitProgram = []( int signal )
    {
        g_GameServerRunning = false;
    };

    std::signal( SIGINT, ExitProgram );
    std::signal( SIGTERM, ExitProgram );

    SetConsoleCtrlHandler( []( DWORD type )
    {
        g_GameServerRunning = false;
        return TRUE;
    }, TRUE );

    while( g_GameServerRunning )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(100) );
    }

    WaitForUserInputToExit.join();

    g_Sockets.Shutdown();

    return 0;
}
