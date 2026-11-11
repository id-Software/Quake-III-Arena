$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$EngineDir = Join-Path $RepoRoot "engine"
$LccDir = Join-Path $RepoRoot "qvmtools/q3lcc"

# These revisions are intentionally pinned, not floating branch heads.
# stb 2b2e64b retains stb_image_resize2.h, which this renderer includes.
# TinyJPEG is pinned to the renderer-compatible revision below.
$Dependencies = @(
    @{
        Name = "stb"
        Path = Join-Path $EngineDir "external/stb"
        Commit = "2b2e64bc4778df6a06fbea3a5dbe61f7a76ecf19"
    },
    @{
        Name = "TinyJPEG"
        Path = Join-Path $EngineDir "external/TinyJPEG"
        Commit = "e978b746714abad76e0f00264d2a154b52de8fc1"
    }
)

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git is required to update engine dependencies."
}

git -C $RepoRoot submodule update --init --recursive -- engine qvmtools/q3lcc
if ($LASTEXITCODE -ne 0) { throw "Failed to initialize engine or QVM compiler submodules." }

foreach ($Dependency in $Dependencies) {
    if (-not (Test-Path (Join-Path $Dependency.Path ".git"))) {
        throw "$($Dependency.Name) submodule is missing at $($Dependency.Path)."
    }

    Write-Host "Updating $($Dependency.Name) to $($Dependency.Commit)..."
    git -C $Dependency.Path fetch --no-tags origin $Dependency.Commit
    if ($LASTEXITCODE -ne 0) {
        throw "Could not fetch pinned $($Dependency.Name) revision."
    }

    git -C $Dependency.Path checkout --detach $Dependency.Commit
    if ($LASTEXITCODE -ne 0) {
        throw "Could not check out pinned $($Dependency.Name) revision."
    }

    $Actual = (git -C $Dependency.Path rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $Actual -ne $Dependency.Commit) {
        throw "$($Dependency.Name) revision verification failed. Expected $($Dependency.Commit), got $Actual."
    }
    Write-Host "$($Dependency.Name) is pinned at $Actual"
}

$ExpectedLcc = "814e3808346aba20f994fa83fca4f9d37124c67c"
$ActualLcc = (git -C $LccDir rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $ActualLcc -ne $ExpectedLcc) {
    throw "QVM LCC revision mismatch. Expected $ExpectedLcc, got $ActualLcc."
}
Write-Host "QVM LCC 4.2 is pinned at $ActualLcc"
Write-Host "All pinned engine dependencies are ready."
