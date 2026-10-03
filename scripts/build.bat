@echo off
REM WattLab Nexus - Firmware Build Script
REM Builds firmware for STM32F746G-DISCO using Arduino CLI

setlocal EnableDelayedExpansion
set "ROOT=%~dp0.."

echo ============================================
echo WattLab Nexus - Firmware Build
echo ============================================

REM Check for arduino-cli
set "ARDUINO_CLI=arduino-cli"
where %ARDUINO_CLI% >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    REM Try Arduino IDE bundled CLI
    if exist "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" (
        set "ARDUINO_CLI=C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    ) else (
        echo ERROR: arduino-cli not found!
        echo Please install Arduino CLI or Arduino IDE
        exit /b 1
    )
)

echo Using: %ARDUINO_CLI%

REM Verify STM32 core is installed
echo Checking STM32 core...
"%ARDUINO_CLI%" core list | findstr /I "STM32" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: STM32 core not found. Installing...
    "%ARDUINO_CLI%" config set board_manager.additional_urls https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
    "%ARDUINO_CLI%" core update-index
    "%ARDUINO_CLI%" core install STMicroelectronics:stm32
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Failed to install STM32 core
        exit /b 1
    )
)

REM Build firmware for STM32F746G-DISCO (pnum=DISCO_F746NG selects the
REM STM32F746NGH6 variant with 1MB flash / 320KB SRAM)
set "NEXUS_FQBN=STMicroelectronics:stm32:Disco:pnum=DISCO_F746NG"

REM Mirror canonical sources into the sketch folder (Arduino only compiles
REM sources that live inside the sketch directory)
echo.
echo Syncing firmware sources into sketch directory...
powershell -ExecutionPolicy Bypass -File "%~dp0sync-sketch.ps1"

echo.
echo Building firmware for STM32F746G-DISCO (%NEXUS_FQBN%)...
pushd "%ROOT%"
"%ARDUINO_CLI%" compile --fqbn "%NEXUS_FQBN%" firmware\wattlab_nexus
set "RC=%ERRORLEVEL%"
popd

if %RC% EQU 0 (
    echo.
    echo ============================================
    echo Build SUCCESSFUL
    echo ============================================
    exit /b 0
) else (
    echo.
    echo ============================================
    echo Build FAILED
    echo ============================================
    exit /b 1
)

endlocal
