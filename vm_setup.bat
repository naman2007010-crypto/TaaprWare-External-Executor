@echo off
echo ========================================
echo   TaaprWare Hypervisor VM Setup
echo ========================================
echo.

echo Step 1: Enabling Test Signing...
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
echo [+] Test signing enabled
echo.

echo Step 2: Verifying AMD-V support...
powershell -Command "(Get-CimInstance Win32_Processor).VirtualizationFirmwareEnabled"
echo.

echo Step 3: Downloading WDK...
echo.
echo Opening Windows Driver Kit download page...
start https://go.microsoft.com/fwlink/?linkid=2196230
echo.
echo MANUAL STEPS:
echo 1. Install Visual Studio 2022 (Community) from the browser
echo 2. Install Windows Driver Kit from the link above
echo 3. After installation, come back to this window
echo.
pause

echo.
echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo NEXT: Reboot the VM for test signing to take effect
echo       After reboot, we'll copy the hypervisor project
echo.
pause
