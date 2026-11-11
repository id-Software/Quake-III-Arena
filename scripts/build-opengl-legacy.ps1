param(
    [string]$Generator = "Visual Studio 17 2022",
    [ValidateSet("x86", "amd64", "x86-64", "all")]
    [string]$Architecture = "amd64",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$LegacyDir = Join-Path $RepoRoot "build\legacy-opengl-source"
$LegacyCommit = "c95f4cecf7e0fec92e275d06645d5f2d14db286c"
$LegacyUrl = "https://github.com/fknfilewalker/Quake-III-Arena-R.git"

foreach ($tool in @("git", "cmake")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "$tool is required for the legacy OpenGL build."
    }
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LegacyDir) | Out-Null
if (-not (Test-Path (Join-Path $LegacyDir ".git"))) {
    if (Test-Path $LegacyDir) { Remove-Item -Recurse -Force $LegacyDir }
    git clone --no-checkout $LegacyUrl $LegacyDir
    if ($LASTEXITCODE -ne 0) { throw "Could not clone the upstream OpenGL-only source." }
}

git -C $LegacyDir fetch origin $LegacyCommit
if ($LASTEXITCODE -ne 0) { throw "Could not fetch pinned OpenGL source revision." }
git -C $LegacyDir checkout --detach $LegacyCommit
if ($LASTEXITCODE -ne 0) { throw "Could not check out pinned OpenGL source revision." }
git -C $LegacyDir submodule update --init --recursive
if ($LASTEXITCODE -ne 0) { throw "Could not initialize legacy OpenGL dependencies." }

if (-not (Test-Path (Join-Path $LegacyDir "CMakeLists.txt"))) {
    throw "Pinned OpenGL source is missing its CMake project."
}

function Build-OpenGL {
    param([string]$Name, [string]$VsArch)

    $BuildDir = Join-Path $RepoRoot ("build-opengl-" + $Name)
    Write-Host ""
    Write-Host "Configuring legacy OpenGL-only engine: $Name ($VsArch)"
    cmake -S $LegacyDir -B $BuildDir -G $Generator -A $VsArch
    if ($LASTEXITCODE -ne 0) { throw "OpenGL CMake configuration failed for $Name." }

    cmake --build $BuildDir --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw "OpenGL engine build failed for $Name." }
}

switch ($Architecture) {
    "x86" { Build-OpenGL -Name "x86" -VsArch "Win32" }
    { $_ -in "amd64", "x86-64" } { Build-OpenGL -Name "x64" -VsArch "x64" }
    "all" {
        Build-OpenGL -Name "x86" -VsArch "Win32"
        Build-OpenGL -Name "x64" -VsArch "x64"
    }
}

Write-Host ""
Write-Host "Legacy OpenGL build outputs are under $LegacyDir\bin and the build directories."
Write-Host "This target is the upstream 'original' branch, pinned at $LegacyCommit; it is separate from the Vulkan/RTX engine."
