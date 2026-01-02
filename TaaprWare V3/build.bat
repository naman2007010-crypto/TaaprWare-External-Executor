@echo off
echo ========================================
echo   Building TaaprWare V3 Internal DLL
echo ========================================

:: Kill existing instances
taskkill /F /IM RobloxPlayerBeta.exe 2>nul

:: Find Visual Studio
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set VS_PATH=%%i

if not defined VS_PATH (
    echo [!] Visual Studio not found!
    pause
    exit /b 1
)

echo [*] Found VS at: %VS_PATH%

:: Initialize build environment
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

:: Create build directory
if not exist "build" mkdir build

echo [*] Compiling...

:: Compile as DLL
cl.exe /nologo /EHsc /std:c++20 /O2 /LD /MT ^
    /I"include" ^
    /Fe:"build\TaaprWareV3.dll" ^
    dllmain.cpp luavm_load_hook.cpp ^
    /link /DLL user32.lib kernel32.lib > build\compile.log 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [!] Build Failed! Check build\compile.log
    type build\compile.log
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo   BUILD SUCCESSFUL!
echo ========================================
echo.

:: Also build the injector
echo [*] Building Injector...
cl.exe /nologo /EHsc /std:c++20 /O2 /MT ^
    /Fe:"build\Injector.exe" ^
    injector.cpp ^
    /link user32.lib kernel32.lib > build\injector.log 2>&1

if %ERRORLEVEL% EQU 0 (
    echo [+] Injector built!
) else (
    echo [!] Injector build failed
)

:: Build GUI
echo [*] Building GUI...
cl.exe /nologo /EHsc /std:c++20 /O2 /MT ^
    /Fe:"build\TaaprWareGUI.exe" ^
    gui.cpp ^
    /link /SUBSYSTEM:WINDOWS user32.lib kernel32.lib ntdll.lib gdi32.lib > build\gui.log 2>&1

if %ERRORLEVEL% EQU 0 (
    echo [+] GUI built!
) else (
    echo [!] GUI build failed
)

echo.
echo DLL: %CD%\build\TaaprWareV3.dll
echo Injector: %CD%\build\Injector.exe
echo GUI: %CD%\build\TaaprWareGUI.exe
echo.
echo HOW TO USE:
echo 1. Start Roblox and join a game
echo 2. Run TaaprWareGUI.exe (as Administrator)
echo 3. Click INJECT, then press DELETE in-game
echo.
pause
