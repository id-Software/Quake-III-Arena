param(
    [string]$Generator = "Visual Studio 17 2022",
    [ValidateSet("x86", "amd64", "x86-64", "all")]
    [string]$Architecture = "amd64",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$EngineDir = Join-Path $RepoRoot "engine"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git is required to initialize the engine submodule."
}
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake 3.14 or newer is required."
}
if (-not $env:VULKAN_SDK -and -not $env:VK_SDK_PATH) {
    throw "Install the Vulkan SDK and set VULKAN_SDK (or VK_SDK_PATH) before building."
}

git -C $RepoRoot submodule update --init --recursive -- engine
if ($LASTEXITCODE -ne 0) { throw "Failed to initialize engine and nested submodules." }

if (-not (Test-Path (Join-Path $EngineDir "CMakeLists.txt"))) {
    throw "The engine submodule is missing. Run: git submodule update --init --recursive"
}

function Build-Target {
    param([string]$Name, [string]$VisualStudioArchitecture)

    $BuildDir = Join-Path $RepoRoot ("build-rtx-" + $Name)
    Write-Host ""
    Write-Host "Configuring $Name ($VisualStudioArchitecture)..."

    cmake -S $EngineDir -B $BuildDir -G $Generator -A $VisualStudioArchitecture
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed for $Name." }

    cmake --build $BuildDir --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw "Engine build failed for $Name." }

    Write-Host "$Name build completed."
}

switch ($Architecture) {
    "x86" {
        Build-Target -Name "x86" -VisualStudioArchitecture "Win32"
    }
    { $_ -in "amd64", "x86-64" } {
        # AMD64 and x86-64 are aliases for the same 64-bit target.
        Build-Target -Name "x64" -VisualStudioArchitecture "x64"
    }
    "all" {
        # Build each distinct architecture once; amd64 and x86-64 are equivalent.
        Build-Target -Name "x86" -VisualStudioArchitecture "Win32"
        Build-Target -Name "x64" -VisualStudioArchitecture "x64"
    }
}

Write-Host ""
Write-Host "Build outputs are under engine/bin and the CMake build directories."
