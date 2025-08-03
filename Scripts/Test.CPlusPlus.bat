@echo off

setlocal

cd ..
cd Client
cd CPlusPlus

if not exist build (
    mkdir build
    cd build
    cmake ..
    cd ..
)

cmake --build build --config Release

pause
