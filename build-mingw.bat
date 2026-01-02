@echo off
echo ========================================
echo   Building TaaprWare External (MinGW)
echo ========================================
echo.

REM Check for g++
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [-] g++ not found. Installing MinGW via winget...
    winget install -e --id GnuWin32.Make
    echo.
    echo [!] Please restart this script after MinGW installation
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build

REM Compile
echo [*] Compiling with g++...
g++ -std=c++20 -O2 -I./include src/main.cpp -o build/TaaprWare-External.exe -static

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [+] Build successful!
    echo [+] Executable: build\TaaprWare-External.exe
    echo.
    echo Run it with: build\TaaprWare-External.exe
    echo.
) else (
    echo.
    echo [-] Build failed!
    echo.
)

pause
