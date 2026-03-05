$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BinDir = Join-Path $ProjectRoot "bin"

if (Test-Path $BinDir) {
    Remove-Item -Path (Join-Path $BinDir "*") -Recurse -Force
    Write-Host "Cleaned $BinDir"
} else {
    Write-Host "Nothing to clean. '$BinDir' does not exist."
}
