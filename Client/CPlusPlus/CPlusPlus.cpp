#include <windows.h>
#include <iostream>

int main()
{
    HMODULE dllHandle = LoadLibraryA( "Client.dll" );

    if( !dllHandle )
    {
        std::cerr << "Failed to load Client.dll\n";
        return 1;
    }

    using StartFunc = void(*)();
    StartFunc StartClient = reinterpret_cast<StartFunc>( GetProcAddress( dllHandle, "StartClient" ) );

    if( !StartClient )
    {
        std::cerr << "Failed to find method \"StartClient\"\n";
        return 1;
    }

    StartClient();

    FreeLibrary( dllHandle );

    return 0;
}
