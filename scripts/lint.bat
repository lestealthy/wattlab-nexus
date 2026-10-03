@echo off
REM WattLab Nexus - Static Analysis Script
setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0lint.ps1"
exit /b %ERRORLEVEL%
