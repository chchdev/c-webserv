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
$ServerOutputFile = Join-Path $BinDir "webserv.exe"
$GuiOutputFile = Join-Path $BinDir "webserv-gui.exe"
$RcFile = Join-Path $ProjectRoot "resources\app.rc"
$RcObjectFile = Join-Path $BinDir "app_icon.o"

if (-not (Test-Path $BinDir)) {
    New-Item -ItemType Directory -Path $BinDir | Out-Null
}

$ServerSources = @(
    (Join-Path $SrcDir "main.c"),
    (Join-Path $SrcDir "webserver.c")
)

$GuiSources = @(
    (Join-Path $SrcDir "gui_main.c")
)

foreach ($SourceFile in ($ServerSources + $GuiSources)) {
    if (-not (Test-Path $SourceFile)) {
        throw "Missing source file: $SourceFile"
    }
}

if (-not (Test-Path $RcFile)) {
    throw "Missing resource file: $RcFile"
}

$Flags = @("-std=c11", "-Wall", "-Wextra", "-I$IncludeDir")
if ($Configuration -eq "Debug") {
    $Flags += @("-g", "-O0")
} else {
    $Flags += @("-O2")
}

Write-Host "Building c-webserv server ($Configuration) with $Compiler..."
& $Compiler @Flags @ServerSources "-o" $ServerOutputFile "-lws2_32"

if ($LASTEXITCODE -ne 0) {
    throw "Server build failed with exit code $LASTEXITCODE."
}

Write-Host "Building c-webserv GUI ($Configuration) with $Compiler..."
& windres $RcFile -O coff -o $RcObjectFile

if ($LASTEXITCODE -ne 0) {
    throw "Resource compilation failed with exit code $LASTEXITCODE."
}

& $Compiler @Flags @GuiSources $RcObjectFile "-o" $GuiOutputFile "-mwindows" "-lws2_32"

if ($LASTEXITCODE -ne 0) {
    throw "GUI build failed with exit code $LASTEXITCODE."
}

Write-Host "Build complete: $ServerOutputFile"
Write-Host "Build complete: $GuiOutputFile"
