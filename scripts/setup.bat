@echo off
REM WattLab Nexus - Environment Setup Script
REM Installs required tools and configures environment

setlocal EnableDelayedExpansion

echo ============================================
echo WattLab Nexus - Environment Setup
echo ============================================

REM Check for Arduino CLI
echo.
echo [1/4] Checking Arduino CLI...
where arduino-cli >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    if exist "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" (
        echo   Found Arduino IDE bundled CLI
        set "ARDUINO_CLI=C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    ) else (
        echo   Arduino CLI not found!
        echo   Please install Arduino IDE 2.x from https://www.arduino.cc/en/software
        exit /b 1
    )
) else (
    echo   Arduino CLI found
    set "ARDUINO_CLI=arduino-cli"
)

REM Check for STM32 core
echo.
echo [2/4] Checking STM32 core...
"%ARDUINO_CLI%" core list | findstr /I "STM32" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   STM32 core not found. Installing...
    "%ARDUINO_CLI%" config set board_manager.additional_urls https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
    "%ARDUINO_CLI%" core update-index
    "%ARDUINO_CLI%" core install STMicroelectronics:stm32
    if %ERRORLEVEL% NEQ 0 (
        echo   ERROR: Failed to install STM32 core
        exit /b 1
    )
) else (
    echo   STM32 core found
)

REM Install required libraries (idempotent).
echo.
echo   Installing libraries...
"%ARDUINO_CLI%" lib install "STM32duino FreeRTOS" >nul 2>&1
"%ARDUINO_CLI%" lib install "STM32duino STM32SD" >nul 2>&1
"%ARDUINO_CLI%" lib install "FatFs" >nul 2>&1
echo   Libraries installed

REM Check for C++ compiler
echo.
echo [3/4] Checking C++ compiler...
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   g++ not found. Installing MinGW-w64...
    winget install --id BrechtSanders.WinLibs.POSIX.UCRT --accept-source-agreements --accept-package-agreements
    if %ERRORLEVEL% NEQ 0 (
        echo   ERROR: Failed to install g++
        exit /b 1
    )
) else (
    echo   g++ found
)

REM Verify installation
echo.
echo [4/4] Verifying installation...
echo   Arduino CLI: "%ARDUINO_CLI%"
"%ARDUINO_CLI%" version
echo.
g++ --version

echo.
echo ============================================
echo Setup complete!
echo ============================================
echo.
echo Next steps:
echo   1. Run scripts\build.bat to build firmware
echo   2. Run scripts\test.bat to run tests
echo   3. Run scripts\build-simulator.bat to build simulator

endlocal
