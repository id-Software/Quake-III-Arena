param([ValidateSet("Generate","Verify")][string]$Mode="Generate",[Parameter(Mandatory=$true)][string]$Path,[string]$Manifest="SHA256SUMS.txt")
$ErrorActionPreference="Stop"
$Root=(Resolve-Path -LiteralPath $Path).Path
$ManifestPath=Join-Path $Root $Manifest
if($Mode -eq "Generate"){
  $lines=@(Get-ChildItem -LiteralPath $Root -File -Recurse | Where-Object {$_.FullName -ne $ManifestPath} | Sort-Object FullName | ForEach-Object {
    $relative=$_.FullName.Substring($Root.TrimEnd([IO.Path]::DirectorySeparatorChar).Length+1).Replace("\","/")
    $hash=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $relative"
  })
  if($lines.Count -eq 0){throw "No files found; refusing to write an empty manifest."}
  [IO.File]::WriteAllLines($ManifestPath,$lines,[Text.UTF8Encoding]::new($false))
  Write-Host "Generated SHA-256 manifest: $ManifestPath ($($lines.Count) files)"
  exit 0
}
if(!(Test-Path -LiteralPath $ManifestPath -PathType Leaf)){throw "Manifest not found: $ManifestPath"}
$failed=0;$checked=0
foreach($line in Get-Content -LiteralPath $ManifestPath){
  if($line -notmatch '^([0-9a-fA-F]{64})  (.+)$'){Write-Error "Malformed manifest line: $line" -ErrorAction Continue;$failed++;continue}
  $expected=$Matches[1].ToLowerInvariant();$relative=$Matches[2]
  $file=Join-Path $Root ($relative.Replace("/",[IO.Path]::DirectorySeparatorChar))
  if(!(Test-Path -LiteralPath $file -PathType Leaf)){Write-Error "MISSING: $relative" -ErrorAction Continue;$failed++;continue}
  $actual=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant();$checked++
  if($actual -ne $expected){Write-Error "FAILED: $relative" -ErrorAction Continue;$failed++}else{Write-Host "OK: $relative"}
}
if($failed){throw "SHA-256 verification failed: $failed problem(s); $checked checked."}
Write-Host "SHA-256 verification passed: $checked file(s)."
