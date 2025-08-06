cmake_minimum_required( VERSION 3.16 )
project( Client )

set( CMAKE_CXX_STANDARD 17 )

add_executable( ${PROJECT_NAME} main.cpp )

add_subdirectory( json )

target_link_libraries( ${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json )
