@echo off

setlocal

cd ..
cd ..
cd Client
cd CPlusPlus

if not exist build (
    mkdir build
    cd build
    cmake ..
    cd ..
)

cmake --build build --config Debug --clean-first

if %ERRORLEVEL% NEQ 0 (
    echo ERROR
    pause
    exit /b %ERRORLEVEL%
)

cd build
cd Debug

Client.exe

pause
