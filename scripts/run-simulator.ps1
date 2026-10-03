# WattLab Nexus - Build and run the host simulator (PowerShell)
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host "ERROR: g++ not found. Install MinGW-w64 (e.g. winget install BrechtSanders.WinLibs.POSIX.UCRT)" -ForegroundColor Red
    exit 1
}

$buildDir = Join-Path $root "tests\build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$include = Join-Path $root "firmware\include"
$simInc  = Join-Path $root "host\simulator"
$mockInc = Join-Path $root "host\mocks"

$sources = @(
    "host\simulator\simulator_main.cpp",
    "host\simulator\virtual_dut.cpp",
    "host\mocks\nexus_storage_host.cpp",
    "firmware\src\nexus_errors.cpp",
    "firmware\src\nexus_log.cpp",
    "firmware\src\nexus_config.cpp",
    "firmware\src\nexus_resource.cpp",
    "firmware\src\nexus_capture.cpp",
    "firmware\src\nexus_device_db.cpp",
    "firmware\src\nexus_test_engine.cpp",
    "firmware\src\nexus_time.cpp",
    "firmware\src\nexus_storage.cpp",
    "firmware\src\nexus_logger.cpp",
    "firmware\src\nexus_persist.cpp",
    "firmware\src\nexus_crash.cpp",
    "firmware\src\nexus_platform.cpp"
)

Write-Host "WattLab Nexus - Building Simulator" -ForegroundColor Cyan
$exe = Join-Path $buildDir "simulator.exe"
$out = Join-Path $buildDir "sim-build.out"
$err = Join-Path $buildDir "sim-build.err"

$gppArgs = @("-std=c++17", "-Wall", "-Wextra",
             "-I$include", "-I$simInc", "-I$mockInc", "-o", $exe) + $sources
$proc = Start-Process -FilePath "g++" -ArgumentList $gppArgs -NoNewWindow -Wait -PassThru `
    -RedirectStandardOutput $out -RedirectStandardError $err
if (Test-Path $err) { Get-Content $err | Out-Host }

if ($proc.ExitCode -ne 0) {
    Write-Host "Simulator build FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "Running simulator..." -ForegroundColor Cyan
& $exe
