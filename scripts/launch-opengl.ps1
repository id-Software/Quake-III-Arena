param(
    [Parameter(Mandatory = $true)]
    [string]$GameExe,
    [string[]]$GameArguments = @()
)

$ErrorActionPreference = "Stop"

$ResolvedExe = (Resolve-Path -LiteralPath $GameExe -ErrorAction Stop).Path
if (-not (Test-Path -LiteralPath $ResolvedExe -PathType Leaf)) {
    throw "Game executable not found: $GameExe"
}

# The renderer's OpenGL driver name on Windows is opengl32.
$Arguments = @("+set", "r_glDriver", "opengl32", "+set", "r_rtx", "0") + $GameArguments
& $ResolvedExe @Arguments
exit $LASTEXITCODE
