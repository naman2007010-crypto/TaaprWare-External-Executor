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
echo DLL: %CD%\build\TaaprWareV3.dll
echo.
echo To inject: Use any DLL injector (Process Hacker, etc.)
echo Target: RobloxPlayerBeta.exe
echo.
pause
