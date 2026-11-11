# Stage legally obtained Quake III Arena 1.32 patch data.
# This copies game-data PK3 files only; it does not download or redistribute them.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PatchDataPath,

    [Parameter(Mandatory = $true)]
    [string]$GamePath
)

$ErrorActionPreference = "Stop"

$patchRoot = (Resolve-Path -LiteralPath $PatchDataPath).Path
$gameRoot = (Resolve-Path -LiteralPath $GamePath).Path
$protocolHeader = Join-Path $PSScriptRoot "..\engine\code\qcommon\qcommon.h"

if (-not (Test-Path -LiteralPath $protocolHeader -PathType Leaf)) {
    throw "Could not find engine protocol header at '$protocolHeader'. Initialize the engine submodule first."
}

$headerText = Get-Content -LiteralPath $protocolHeader -Raw
if ($headerText -notmatch '(?m)^\s*#define\s+PROTOCOL_VERSION\s+68\b') {
    throw "The selected engine source does not declare Quake III Arena protocol 68. Refusing to stage 1.32 patch data."
}

$requiredDirectories = @("baseq3", "missionpack")
foreach ($gameDir in $requiredDirectories) {
    $sourceDir = Join-Path $patchRoot $gameDir
    $destinationDir = Join-Path $gameRoot $gameDir

    if (-not (Test-Path -LiteralPath $sourceDir -PathType Container)) {
        throw "Patch data directory '$sourceDir' is missing."
    }
    if (-not (Test-Path -LiteralPath $destinationDir -PathType Container)) {
        throw "Game data directory '$destinationDir' is missing. Install the original game/Team Arena data first."
    }

    $patchFiles = @(Get-ChildItem -LiteralPath $sourceDir -Filter "pak*.pk3" -File)
    if ($patchFiles.Count -eq 0) {
        throw "No pak*.pk3 patch files were found in '$sourceDir'."
    }

    if ($gameDir -eq "baseq3") {
        $missing = 1..8 | Where-Object {
            -not (Test-Path -LiteralPath (Join-Path $sourceDir ("pak{0}.pk3" -f $_)) -PathType Leaf)
        }
        if ($missing.Count -gt 0) {
            throw "The baseq3 patch data is incomplete. Missing: $($missing -join ', '). Use the official 1.32 patch-data set."
        }
    }

    foreach ($file in $patchFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination $destinationDir -Force
        Write-Host ("Installed {0}\{1}" -f $gameDir, $file.Name)
    }
}

Write-Host "Quake III Arena 1.32 patch data staged successfully."
Write-Host "Engine source protocol check: 68 (official 1.32 / 1.32c network protocol)."
Write-Host "This does not install the original 1.32c executable or independently certify the 1.32c security fixes."
