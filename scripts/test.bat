@echo off
REM WattLab Nexus - Host Test Script
REM Delegates to the PowerShell runner so the two stay in sync.

setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0run-tests.ps1"
exit /b %ERRORLEVEL%
