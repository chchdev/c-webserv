param(
    [string]$Compiler = "gcc",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$SrcDir = Join-Path $ProjectRoot "src"
$IncludeDir = Join-Path $ProjectRoot "include"
$BinDir = Join-Path $ProjectRoot "bin"
$OutputFile = Join-Path $BinDir "webserv.exe"

if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Path $BinDir | Out-Null
}

$Sources = Get-ChildItem -Path $SrcDir -Filter "*.c" | ForEach-Object { $_.FullName }
if (-not $Sources) {
    throw "No C source files found in '$SrcDir'."
}

$Flags = @("-std=c11", "-Wall", "-Wextra", "-I$IncludeDir")
if ($Configuration -eq "Debug") {
    $Flags += @("-g", "-O0")
} else {
    $Flags += @("-O2")
}

Write-Host "Building c-webserv ($Configuration) with $Compiler..."
& $Compiler @Flags @Sources "-o" $OutputFile "-lws2_32"

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

Write-Host "Build complete: $OutputFile"
