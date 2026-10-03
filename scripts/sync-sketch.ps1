# WattLab Nexus - Sync firmware sources into the Arduino sketch directory.
#
# The Arduino build system only compiles sources that live inside the sketch
# folder, so this mirrors firmware/include and firmware/src into
# firmware/wattlab_nexus. The canonical sources remain under src/ and include/.
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$includeDir = Join-Path $root "firmware\include"
$srcDir = Join-Path $root "firmware\src"
$sketchDir = Join-Path $root "firmware\wattlab_nexus"

New-Item -ItemType Directory -Force -Path $sketchDir | Out-Null

# Remove previously synced copies (keep the .ino sketch file itself)
Get-ChildItem -Path $sketchDir -File | Where-Object { $_.Extension -in ".h", ".cpp" } | Remove-Item -Force

Copy-Item (Join-Path $includeDir "*.h") $sketchDir -Force
Copy-Item (Join-Path $srcDir "*.cpp") $sketchDir -Force

$count = (Get-ChildItem $sketchDir -File).Count
Write-Host "Synced sources to $sketchDir ($count files)"
