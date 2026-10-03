@echo off
REM WattLab Nexus - Simulator Build Script
REM Delegates to the PowerShell runner so the two stay in sync.

setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0run-simulator.ps1"
exit /b %ERRORLEVEL%
