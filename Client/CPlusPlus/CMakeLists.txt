cmake_minimum_required( VERSION 3.16 )
project( Client )

set( CMAKE_CXX_STANDARD 17 )

add_library( ${PROJECT_NAME} SHARED
    SocketServer.cpp
    SocketServer.h

    # Testing purposes
    main.cpp
)

add_executable( test CPlusPlus.cpp )
