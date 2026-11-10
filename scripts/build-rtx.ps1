param(
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Architecture = "x64",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$EngineDir = Join-Path $RepoRoot "engine"
$BuildDir = Join-Path $RepoRoot "build-rtx"

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

cmake -S $EngineDir -B $BuildDir -G $Generator -A $Architecture
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

cmake --build $BuildDir --config $Configuration
if ($LASTEXITCODE -ne 0) { throw "Engine build failed." }

Write-Host "Build completed. Check engine/bin for the generated executable and libraries."
