@echo off
REM WattLab Nexus - Build All Script
REM Builds firmware, runs tests, and builds simulator

setlocal EnableDelayedExpansion

echo ============================================
echo WattLab Nexus - Build All
echo ============================================

REM Build firmware
echo.
echo [1/3] Building firmware...
call ..\scripts\build.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Firmware build FAILED
    exit /b 1
)

REM Run tests
echo.
echo [2/3] Running tests...
call ..\scripts\test.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Tests FAILED
    exit /b 1
)

REM Build simulator
echo.
echo [3/3] Building simulator...
call ..\scripts\build-simulator.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Simulator build FAILED
    exit /b 1
)

echo.
echo ============================================
echo All builds completed successfully
echo ============================================

endlocal
