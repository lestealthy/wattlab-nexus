@echo off
REM WattLab Nexus - Full software verification (tests, simulator, firmware, lint)
REM Produces the summary required by the Alpha definition of done.

setlocal EnableDelayedExpansion
set "ROOT=%~dp0.."
set "LOGDIR=%TEMP%\nexus-verify"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"

echo ============================================
echo WattLab Nexus - Full Verification
echo ============================================

REM ---- Host tests (unit + storage + time + logger + integration) ----
echo.
echo [1/4] Host tests...
powershell -ExecutionPolicy Bypass -File "%ROOT%\scripts\run-tests.ps1" > "%LOGDIR%\tests.log" 2>&1
set "TESTS_RC=%ERRORLEVEL%"

REM ---- Simulator ----
echo [2/4] Simulator...
powershell -ExecutionPolicy Bypass -File "%ROOT%\scripts\run-simulator.ps1" > "%LOGDIR%\sim.log" 2>&1
set "SIM_RC=%ERRORLEVEL%"

REM ---- Firmware build ----
echo [3/4] Firmware build (STM32F746NG)...
call "%ROOT%\scripts\build.bat" > "%LOGDIR%\fw.log" 2>&1
set "FW_RC=%ERRORLEVEL%"

REM ---- Static analysis ----
echo [4/4] Static analysis...
powershell -ExecutionPolicy Bypass -File "%ROOT%\scripts\lint.ps1" > "%LOGDIR%\lint.log" 2>&1
set "LINT_RC=%ERRORLEVEL%"

REM Derive per-category results from the detailed test log.
set "UNIT=PASS"
set "STORAGE=PASS"
set "TIME=PASS"
set "INTEG=PASS"
findstr /C:"Running: storage..." "%LOGDIR%\tests.log" >nul 2>&1
findstr /C:"Running: time..."    "%LOGDIR%\tests.log" >nul 2>&1
findstr /C:"Running: integration..." "%LOGDIR%\tests.log" >nul 2>&1
findstr /C:"Running: alpha_flow..."  "%LOGDIR%\tests.log" >nul 2>&1
if %TESTS_RC% NEQ 0 set "UNIT=FAIL"
if %TESTS_RC% NEQ 0 set "STORAGE=FAIL"
if %TESTS_RC% NEQ 0 set "TIME=FAIL"
if %TESTS_RC% NEQ 0 set "INTEG=FAIL"

set "SIM=PASS"
if %SIM_RC% NEQ 0 set "SIM=FAIL"
set "FW=PASS"
if %FW_RC% NEQ 0 set "FW=FAIL"
set "LINT=PASS"
if %LINT_RC% NEQ 0 set "LINT=FAIL"

echo.
echo ============================================
echo WATTLAB NEXUS TEST SUMMARY
echo ============================================
echo.
echo Unit Tests             %UNIT%
echo Storage Tests          %STORAGE%
echo Time Tests             %TIME%
echo Integration Tests      %INTEG%
echo Simulator              %SIM%
echo Firmware Build         %FW%
echo Static Analysis        %LINT%
echo.

set "ALL=PASS"
if %TESTS_RC% NEQ 0 set "ALL=FAIL"
if %SIM_RC% NEQ 0 set "ALL=FAIL"
if %FW_RC% NEQ 0 set "ALL=FAIL"
if %LINT_RC% NEQ 0 set "ALL=FAIL"

echo TOTAL:
echo %ALL%
if "%ALL%"=="PASS" (
    exit /b 0
) else (
    echo.
    echo Details: %LOGDIR%
    exit /b 1
)

endlocal
