@echo off
REM WattLab Nexus - Clean build artifacts (resolves paths relative to this script)

setlocal
set "ROOT=%~dp0.."

echo Cleaning build artifacts...

if exist "%ROOT%\tests\build" (
    rmdir /s /q "%ROOT%\tests\build"
    echo Removed tests\build
)

if exist "%ROOT%\test-results" (
    rmdir /s /q "%ROOT%\test-results"
    echo Removed test-results
)

echo Clean complete.
endlocal
