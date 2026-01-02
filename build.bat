@echo off
echo ========================================
echo   Building TaaprWare External
echo ========================================
echo.

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Navigate to project root
cd /d "c:\Users\naman\Desktop\Bnation\roblox\TaaprWare-External"

REM Define paths
set SRC_DIR=%CD%\src
set INC_DIR=%CD%\include
set BUILD_DIR=%CD%\build

echo Source: %SRC_DIR%\main.cpp
echo Include: %INC_DIR%

if not exist "%SRC_DIR%\main.cpp" (
    echo [-] Error: main.cpp not found at %SRC_DIR%\main.cpp
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Compile with full paths
echo [*] Compiling...
cl.exe /EHsc /std:c++17 /DWIN32 /D_WINDOWS /O2 /MT /I"%INC_DIR%" "%SRC_DIR%\main.cpp" /Fe:TaaprWare-External.exe /link /SUBSYSTEM:CONSOLE

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo   BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Executable: %BUILD_DIR%\TaaprWare-External.exe
    echo.
) else (
    echo.
    echo ========================================
    echo   BUILD FAILED
    echo ========================================
    echo.
)

pause
