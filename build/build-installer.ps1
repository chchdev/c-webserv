param(
    [string]$Compiler = "gcc",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$InstallerScript = Join-Path $ProjectRoot "installer\c-webserv.iss"

function Resolve-IsccPath {
    $command = Get-Command "iscc" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidates = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

$IsccExe = Resolve-IsccPath

Write-Host "Building binaries before installer..."
& (Join-Path $PSScriptRoot "build.ps1") -Compiler $Compiler -Configuration $Configuration

if ($LASTEXITCODE -ne 0) {
    throw "Binary build failed with exit code $LASTEXITCODE."
}

if (-not $IsccExe) {
    throw "Inno Setup compiler not found. Install Inno Setup 6 or add ISCC.exe to PATH."
}

if (-not (Test-Path $InstallerScript)) {
    throw "Installer script not found: $InstallerScript"
}

Write-Host "Compiling installer with Inno Setup..."
& $IsccExe $InstallerScript

if ($LASTEXITCODE -ne 0) {
    throw "Installer build failed with exit code $LASTEXITCODE."
}

$OutputDir = Join-Path $ProjectRoot "bin\installer"
Write-Host "Installer build complete. Output directory: $OutputDir"
