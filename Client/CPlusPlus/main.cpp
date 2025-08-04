#include "SocketServer.h"
#include <atomic>

std::atomic<bool> g_Running = true;

void SimulateGameRunning()
{
    std::cin.get();
    g_Running = false;
}


extern "C" __declspec( dllexport ) void StartClient()
{
    std::thread waitThread( SimulateGameRunning );

    std::cout << "Press Enter at any moment to exit\n";

    g_Sockets.Send( "Hello world from the game server!\n" );

    int time = 0;

    while( g_Running )
    {
        std::this_thread::sleep_for( std::chrono::seconds(5) );

        continue;

        char msg[GLDSOURCE_CHAT_MAX_CHARS];
        sprintf( msg, "test %i\n", time );
        g_Sockets.Send( msg );
        time++;
    }

    g_Sockets.Shutdown();
    waitThread.join();
}
