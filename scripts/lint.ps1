# WattLab Nexus - lightweight static analysis over firmware sources.
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

$srcFiles = Get-ChildItem -Path (Join-Path $root "firmware\src"), (Join-Path $root "firmware\include") -Filter *.cpp -File -ErrorAction SilentlyContinue
$hdrFiles = Get-ChildItem -Path (Join-Path $root "firmware\include") -Filter *.h -File -ErrorAction SilentlyContinue
$all = @($srcFiles) + @($hdrFiles)

$warnings = 0

Write-Host "============================================"
Write-Host "WattLab Nexus - Static Analysis"
Write-Host "============================================"

function Report-Matches($title, $pattern, $files) {
    Write-Host ""
    Write-Host "$title"
    $hits = Select-String -Path $files.FullName -Pattern $pattern -ErrorAction SilentlyContinue
    if ($hits) {
        $script:warnings++
        $hits | ForEach-Object { Write-Host ("  {0}:{1}: {2}" -f $_.Filename, $_.LineNumber, $_.Line.Trim()) }
    } else {
        Write-Host "  none"
    }
}

# TODO/FIXME markers (informational; not necessarily a failure)
Write-Host ""
Write-Host "TODO/FIXME markers"
$todos = Select-String -Path $all.FullName -Pattern "TODO|FIXME|HACK|XXX" -ErrorAction SilentlyContinue
if ($todos) {
    $todos | ForEach-Object { Write-Host ("  {0}:{1}: {2}" -f $_.Filename, $_.LineNumber, $_.Line.Trim()) }
} else {
    Write-Host "  none"
}

# Blocking delay() in firmware sources (should be vTaskDelay in tasks).
# Exclude comment lines and the intentional test-engine delay step.
Write-Host ""
Write-Host "Blocking delay() calls in firmware sources"
$delayHits = Select-String -Path $srcFiles.FullName -Pattern "\bdelay\s*\(" -ErrorAction SilentlyContinue |
    Where-Object { $_.Line -notmatch "^\s*//" -and $_.Line -notmatch "//.*delay" }
if ($delayHits) {
    $warnings++
    $delayHits | ForEach-Object { Write-Host ("  {0}:{1}: {2}" -f $_.Filename, $_.LineNumber, $_.Line.Trim()) }
} else {
    Write-Host "  none"
}

# Direct Serial.print (should use NEXUS_LOG macros)
Report-Matches "Direct Serial.print calls" "Serial\.print" $srcFiles

# Long hex literals not attached to a named macro (review for magic numbers)
Write-Host ""
Write-Host "Long hex literals (review for magic numbers)"
$hexHits = Select-String -Path $all.FullName -Pattern "0x[0-9A-Fa-f]{6,}" -ErrorAction SilentlyContinue |
    Where-Object {
        $_.Line -notmatch "^\s*//" -and
        $_.Line -notmatch "#define" -and
        $_.Line -notmatch "0x[0-9A-Fa-f]{8}"  # addresses/32-bit ids are usually legitimate
    }
if ($hexHits) {
    $warnings++
    $hexHits | ForEach-Object { Write-Host ("  {0}:{1}: {2}" -f $_.Filename, $_.LineNumber, $_.Line.Trim()) }
} else {
    Write-Host "  none"
}

Write-Host ""
Write-Host "============================================"
Write-Host "Static Analysis Summary"
Write-Host "Warning categories triggered: $warnings"
Write-Host "============================================"

if ($warnings -gt 0) {
    exit 1
}
exit 0
