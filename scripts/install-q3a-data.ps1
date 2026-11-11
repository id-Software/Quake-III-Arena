# Stage legally obtained Quake III Arena retail data. No game data is downloaded or redistributed.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourceDataRoot,
    [Parameter(Mandatory = $true)][string]$GameInstallRoot
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression.FileSystem

$source = (Resolve-Path -LiteralPath $SourceDataRoot).Path
$destination = [System.IO.Path]::GetFullPath($GameInstallRoot)
$baseSource = Join-Path $source "baseq3"
$baseDestination = Join-Path $destination "baseq3"
if (-not (Test-Path -LiteralPath $baseSource -PathType Container)) {
    throw "Source baseq3 directory is missing: $baseSource"
}
New-Item -ItemType Directory -Force -Path $baseDestination | Out-Null

function Test-Pk3([string]$Path) {
    try {
        $archive = [System.IO.Compression.ZipFile]::OpenRead($Path)
        try {
            if ($archive.Entries.Count -eq 0) { throw "Archive is empty." }
            foreach ($entry in $archive.Entries) {
                $stream = $entry.Open()
                try {
                    $buffer = New-Object byte[] 65536
                    while ($stream.Read($buffer, 0, $buffer.Length) -gt 0) { }
                } finally { $stream.Dispose() }
            }
        } finally { $archive.Dispose() }
    } catch {
        throw "Invalid PK3 '$Path': $($_.Exception.Message)"
    }
}

foreach ($number in 0..8) {
    $name = "pak$number.pk3"
    $path = Join-Path $baseSource $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing required Quake III Arena data archive: $path"
    }
    Test-Pk3 $path
}
foreach ($number in 0..8) {
    $name = "pak$number.pk3"
    Copy-Item -LiteralPath (Join-Path $baseSource $name) -Destination (Join-Path $baseDestination $name) -Force
    Write-Host "Installed baseq3/$name"
}

$missionSource = Join-Path $source "missionpack"
if (Test-Path -LiteralPath $missionSource -PathType Container) {
    $missionFiles = @(Get-ChildItem -LiteralPath $missionSource -Filter "pak*.pk3" -File)
    if ($missionFiles.Count -gt 0) {
        $missionDestination = Join-Path $destination "missionpack"
        New-Item -ItemType Directory -Force -Path $missionDestination | Out-Null
        foreach ($file in $missionFiles) {
            Test-Pk3 $file.FullName
            Copy-Item -LiteralPath $file.FullName -Destination $missionDestination -Force
            Write-Host "Installed missionpack/$($file.Name)"
        }
    }
}
Write-Host "Quake III Arena data staged at: $destination"
Write-Host "Team Arena data is included only if missionpack PK3 files were supplied."
