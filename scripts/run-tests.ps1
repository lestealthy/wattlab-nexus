# WattLab Nexus - Cross-platform test runner (PowerShell)
# Compiles and runs the host test suite, producing machine-readable reports.

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

# Locate a C++ compiler
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host "ERROR: g++ not found. Install MinGW-w64 (e.g. winget install BrechtSanders.WinLibs.POSIX.UCRT)" -ForegroundColor Red
    exit 1
}

$buildDir = Join-Path $root "tests\build"
$resultsDir = Join-Path $root "test-results"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $resultsDir | Out-Null

$fwSrc = Join-Path $root "firmware\src"
$include = Join-Path $root "firmware\include"

$sources = @(
    "tests\unit\test_main.cpp",
    "tests\unit\test_config.cpp",
    "tests\unit\test_capture.cpp",
    "tests\unit\test_device_db.cpp",
    "tests\unit\test_test_engine.cpp",
    "tests\unit\test_resource.cpp",
    "tests\unit\test_decoders.cpp",
    "tests\unit\test_measurement.cpp",
    "tests\unit\test_message.cpp",
    "tests\unit\test_crash.cpp",
    "tests\unit\test_time.cpp",
    "tests\unit\test_storage.cpp",
    "tests\unit\test_logger.cpp",
    "tests\integration\test_integration.cpp",
    "tests\integration\test_alpha_flow.cpp",
    "firmware\src\nexus_errors.cpp",
    "firmware\src\nexus_log.cpp",
    "firmware\src\nexus_config.cpp",
    "firmware\src\nexus_resource.cpp",
    "firmware\src\nexus_capture.cpp",
    "firmware\src\nexus_device_db.cpp",
    "firmware\src\nexus_test_engine.cpp",
    "firmware\src\nexus_decoders.cpp",
    "firmware\src\nexus_measurement.cpp",
    "firmware\src\nexus_calibration.cpp",
    "firmware\src\nexus_message.cpp",
    "firmware\src\nexus_crash.cpp",
    "firmware\src\nexus_time.cpp",
    "firmware\src\nexus_storage.cpp",
    "firmware\src\nexus_logger.cpp",
    "firmware\src\nexus_persist.cpp",
    "firmware\src\nexus_boot.cpp",
    "firmware\src\nexus_platform.cpp",
    "host\mocks\nexus_storage_host.cpp"
)

Write-Host "WattLab Nexus - Host Test Suite" -ForegroundColor Cyan
Write-Host "Compiling tests..."

$exe = Join-Path $buildDir "test_runner.exe"
$compileLog = Join-Path $resultsDir "compile.log"
$compileErr = Join-Path $resultsDir "compile-err.log"

# Invoke the compiler with stderr redirected to a file. Native stderr must not
# be interpreted as a PowerShell terminating error under ErrorActionPreference.
$gppArgs = @("-std=c++17", "-Wall", "-Wextra", "-Werror=return-type",
          "-I$include", "-I$(Join-Path $root 'host\mocks')", "-o", $exe) + $sources
$proc = Start-Process -FilePath "g++" -ArgumentList $gppArgs -NoNewWindow -Wait -PassThru `
    -RedirectStandardOutput $compileLog -RedirectStandardError $compileErr
$compileOutput = @()
if (Test-Path $compileLog) { $compileOutput += Get-Content $compileLog }
if (Test-Path $compileErr) { $compileOutput += Get-Content $compileErr }
$compileOutput | Set-Content -Path (Join-Path $resultsDir "compile.log")

if ($proc.ExitCode -ne 0) {
    Write-Host "Test compilation FAILED" -ForegroundColor Red
    if ($compileOutput) { $compileOutput | Out-Host }
    exit 1
}
if ($compileOutput) { $compileOutput | Out-Host }

Write-Host "Running tests..." -ForegroundColor Cyan
$output = & $exe 2>&1 | Tee-Object -FilePath (Join-Path $resultsDir "test-output.txt")
$output | Out-Host

$exitCode = $LASTEXITCODE

# Parse summary line: "Results: N passed, M failed, T total"
$summary = $output | Select-String -Pattern "Results:\s+(\d+) passed,\s+(\d+) failed,\s+(\d+) total" | Select-Object -First 1

$passed = 0; $failed = 0; $total = 0
if ($summary) {
    $passed = [int]$summary.Matches[0].Groups[1].Value
    $failed = [int]$summary.Matches[0].Groups[2].Value
    $total  = [int]$summary.Matches[0].Groups[3].Value
}

# Emit machine-readable JSON report
$report = [ordered]@{
    project   = "WattLab Nexus"
    version   = "0.1.0"
    timestamp = (Get-Date).ToUniversalTime().ToString("o")
    suite     = "host"
    passed    = $passed
    failed    = $failed
    total     = $total
    result    = if ($failed -eq 0 -and $total -gt 0) { "PASS" } else { "FAIL" }
}
$json = $report | ConvertTo-Json -Depth 4
Set-Content -Path (Join-Path $resultsDir "test-results.json") -Value $json

# Human readable summary
$txt = @"
WattLab Nexus - Host Test Report
================================
Date:     $($report.timestamp)
Passed:   $passed
Failed:   $failed
Total:    $total
Result:   $($report.result)
"@
Set-Content -Path (Join-Path $resultsDir "test-results.txt") -Value $txt

Write-Host ""
if ($failed -eq 0 -and $total -gt 0) {
    Write-Host "All tests PASSED ($passed/$total)" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Tests FAILED ($failed failed of $total)" -ForegroundColor Red
    exit 1
}
