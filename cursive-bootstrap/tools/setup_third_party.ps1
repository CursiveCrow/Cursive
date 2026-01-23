<#
.SYNOPSIS
    Downloads and sets up third-party dependencies for the Cursive 0 compiler.

.DESCRIPTION
    This script downloads and extracts:
    - LLVM 21.1.8 (prebuilt Windows binaries)
    - ICU 72.1 (prebuilt Windows MSVC2019 binaries)
    - tomlplusplus 3.4.0 (header-only library)

    Dependencies are placed in the third_party/ directory with the structure
    expected by CMakeLists.txt.

.PARAMETER Arch
    Target architecture: "x64" or "arm64". Defaults to auto-detect based on current system.

.PARAMETER Force
    Re-download and extract dependencies even if they already exist.

.EXAMPLE
    .\setup_third_party.ps1
    Downloads all dependencies for the current architecture if not already present.

.EXAMPLE
    .\setup_third_party.ps1 -Arch arm64
    Downloads ARM64 dependencies.

.EXAMPLE
    .\setup_third_party.ps1 -Force
    Re-downloads all dependencies, overwriting existing files.
#>

[CmdletBinding()]
param(
    [ValidateSet("x64", "arm64", "auto")]
    [string]$Arch = "auto",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Configuration
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ThirdPartyDir = Join-Path $ScriptDir "third_party"
$TempDir = Join-Path $ThirdPartyDir "temp"

# Auto-detect architecture
if ($Arch -eq "auto") {
    $cpuArch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
    if ($cpuArch -eq "Arm64") {
        $Arch = "arm64"
    } else {
        $Arch = "x64"
    }
    Write-Host "Auto-detected architecture: $Arch" -ForegroundColor Cyan
}

# Architecture-specific configuration
$ArchConfig = @{
    "x64" = @{
        LLVMTriple = "x86_64-pc-windows-msvc"
        LLVMDirName = "llvm-21.1.8-x86_64"
        ICUArchive = "icu4c-72_1-Win64-MSVC2019.zip"
        ICUDir = "win64"
    }
    "arm64" = @{
        LLVMTriple = "aarch64-pc-windows-msvc"
        LLVMDirName = "llvm-21.1.8-aarch64"
        ICUArchive = "icu4c-72_1-WinARM64-MSVC2019.zip"
        ICUDir = "winarm64"
    }
}

$CurrentArch = $ArchConfig[$Arch]

# Dependency URLs and paths
$Dependencies = @{
    LLVM = @{
        Version = "21.1.8"
        Url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.8/clang+llvm-21.1.8-$($CurrentArch.LLVMTriple).tar.xz"
        TargetDir = Join-Path $ThirdPartyDir "llvm\$($CurrentArch.LLVMDirName)"
        ArchiveName = "clang+llvm-21.1.8-$($CurrentArch.LLVMTriple).tar.xz"
        ValidationFile = "lib\cmake\llvm\LLVMConfig.cmake"
    }
    ICU = @{
        Version = "72.1"
        Url = "https://github.com/unicode-org/icu/releases/download/release-72-1/$($CurrentArch.ICUArchive)"
        TargetDir = Join-Path $ThirdPartyDir "icu\$($CurrentArch.ICUDir)"
        ArchiveName = $CurrentArch.ICUArchive
        ValidationFile = "lib64\icuuc.lib"
    }
    TomlPlusPlus = @{
        Version = "3.4.0"
        Url = "https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.zip"
        TargetDir = Join-Path $ThirdPartyDir "tomlplusplus"
        ArchiveName = "tomlplusplus-3.4.0.zip"
        ValidationFile = "include\toml++\toml.hpp"
    }
}

function Write-Status {
    param([string]$Message, [string]$Color = "Cyan")
    Write-Host ">>> $Message" -ForegroundColor $Color
}

function Write-Success {
    param([string]$Message)
    Write-Host "  [OK] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "  [WARN] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "  [ERROR] $Message" -ForegroundColor Red
}

function Test-CommandExists {
    param([string]$Command)
    $null -ne (Get-Command $Command -ErrorAction SilentlyContinue)
}

function Get-7ZipPath {
    # Check for 7z in PATH
    if (Test-CommandExists "7z") {
        return "7z"
    }
    
    # Check common installation paths
    $paths = @(
        "C:\Program Files\7-Zip\7z.exe",
        "C:\Program Files (x86)\7-Zip\7z.exe",
        "$env:ProgramFiles\7-Zip\7z.exe",
        "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
    )
    
    foreach ($path in $paths) {
        if (Test-Path $path) {
            return $path
        }
    }
    
    return $null
}

function Expand-TarXz {
    param(
        [string]$ArchivePath,
        [string]$DestinationPath
    )
    
    $7z = Get-7ZipPath
    if (-not $7z) {
        throw "7-Zip is required to extract .tar.xz files. Please install 7-Zip from https://www.7-zip.org/"
    }
    
    $tarFile = $ArchivePath -replace '\.xz$', ''
    
    Write-Host "    Extracting .xz with 7-Zip..." -ForegroundColor Gray
    & $7z x $ArchivePath -o"$(Split-Path $ArchivePath -Parent)" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to extract .xz archive"
    }
    
    Write-Host "    Extracting .tar with 7-Zip..." -ForegroundColor Gray
    & $7z x $tarFile -o"$DestinationPath" -y | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to extract .tar archive"
    }
    
    # Clean up intermediate tar file
    Remove-Item $tarFile -Force -ErrorAction SilentlyContinue
}

function Expand-ZipArchive {
    param(
        [string]$ArchivePath,
        [string]$DestinationPath
    )
    
    Write-Host "    Extracting .zip..." -ForegroundColor Gray
    Expand-Archive -Path $ArchivePath -DestinationPath $DestinationPath -Force
}

function Get-Dependency {
    param(
        [string]$Name,
        [hashtable]$Config
    )
    
    $targetDir = $Config.TargetDir
    $validationPath = Join-Path $targetDir $Config.ValidationFile
    
    # Check if already installed
    if ((Test-Path $validationPath) -and -not $Force) {
        Write-Success "$Name $($Config.Version) already installed"
        return $true
    }
    
    Write-Status "Downloading $Name $($Config.Version)..."
    
    # Create temp directory
    if (-not (Test-Path $TempDir)) {
        New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
    }
    
    $archivePath = Join-Path $TempDir $Config.ArchiveName
    
    # Download
    try {
        Write-Host "    URL: $($Config.Url)" -ForegroundColor Gray
        
        # Use TLS 1.2 for GitHub
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        
        $webClient = New-Object System.Net.WebClient
        $webClient.Headers.Add("User-Agent", "Cursive-Setup-Script/1.0")
        $webClient.DownloadFile($Config.Url, $archivePath)
        
        Write-Success "Downloaded $($Config.ArchiveName)"
    }
    catch {
        Write-Error "Failed to download $Name : $_"
        return $false
    }
    
    # Extract
    Write-Status "Extracting $Name..."
    
    try {
        # Create parent directory
        $parentDir = Split-Path $targetDir -Parent
        if (-not (Test-Path $parentDir)) {
            New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
        }
        
        # Remove existing target directory if Force is set
        if ($Force -and (Test-Path $targetDir)) {
            Remove-Item $targetDir -Recurse -Force
        }
        
        # Create extraction temp directory
        $extractTemp = Join-Path $TempDir "extract_$Name"
        if (Test-Path $extractTemp) {
            Remove-Item $extractTemp -Recurse -Force
        }
        New-Item -ItemType Directory -Path $extractTemp -Force | Out-Null
        
        # Extract based on file type
        if ($archivePath -match '\.tar\.xz$') {
            Expand-TarXz -ArchivePath $archivePath -DestinationPath $extractTemp
        }
        elseif ($archivePath -match '\.zip$') {
            Expand-ZipArchive -ArchivePath $archivePath -DestinationPath $extractTemp
        }
        else {
            throw "Unknown archive format: $archivePath"
        }
        
        # Handle different extraction structures
        switch ($Name) {
            "LLVM" {
                # LLVM extracts to clang+llvm-21.1.8-x86_64-pc-windows-msvc/
                $extractedDir = Get-ChildItem $extractTemp -Directory | Where-Object { $_.Name -match "^clang\+llvm" } | Select-Object -First 1
                if ($extractedDir) {
                    if (Test-Path $targetDir) {
                        Remove-Item $targetDir -Recurse -Force
                    }
                    Move-Item $extractedDir.FullName $targetDir
                }
                else {
                    throw "Could not find extracted LLVM directory"
                }
            }
            "ICU" {
                # ICU from GitHub releases is double-zipped:
                # icu4c-72_1-Win64-MSVC2019.zip -> 20221013.8_ICU4C_MSVC_x64_Release/icu-windows.zip -> bin64/, include/, lib64/
                $outerDir = Get-ChildItem $extractTemp -Directory | Select-Object -First 1
                if (-not $outerDir) {
                    throw "Could not find extracted ICU outer directory"
                }
                
                $innerZip = Get-ChildItem $outerDir.FullName -Filter "*.zip" | Select-Object -First 1
                if ($innerZip) {
                    # Extract inner zip
                    $innerExtract = Join-Path $extractTemp "icu_inner"
                    Expand-Archive -Path $innerZip.FullName -DestinationPath $innerExtract -Force
                    
                    # Move contents to target
                    if (Test-Path $targetDir) {
                        Remove-Item $targetDir -Recurse -Force
                    }
                    Move-Item $innerExtract $targetDir
                }
                else {
                    # Fallback: maybe it's a direct extraction with bin64/, include/, lib64/
                    $hasDirectContent = (Test-Path (Join-Path $outerDir.FullName "bin64")) -or 
                                       (Test-Path (Join-Path $outerDir.FullName "include"))
                    if ($hasDirectContent) {
                        if (Test-Path $targetDir) {
                            Remove-Item $targetDir -Recurse -Force
                        }
                        Move-Item $outerDir.FullName $targetDir
                    }
                    else {
                        throw "Could not find ICU content (expected icu-windows.zip or bin64/include/lib64 directories)"
                    }
                }
            }
            "TomlPlusPlus" {
                # tomlplusplus extracts to tomlplusplus-3.4.0/ with include/ subfolder
                $extractedDir = Get-ChildItem $extractTemp -Directory | Where-Object { $_.Name -match "^tomlplusplus" } | Select-Object -First 1
                if ($extractedDir) {
                    if (Test-Path $targetDir) {
                        Remove-Item $targetDir -Recurse -Force
                    }
                    Move-Item $extractedDir.FullName $targetDir
                }
                else {
                    throw "Could not find extracted tomlplusplus directory"
                }
            }
        }
        
        # Clean up
        Remove-Item $extractTemp -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item $archivePath -Force -ErrorAction SilentlyContinue
        
        Write-Success "Extracted $Name to $targetDir"
    }
    catch {
        Write-Error "Failed to extract $Name : $_"
        return $false
    }
    
    return $true
}

function Test-Dependencies {
    Write-Status "Verifying installed dependencies..." "Yellow"
    
    $allValid = $true
    
    # LLVM validation
    $llvmConfig = Join-Path $Dependencies.LLVM.TargetDir $Dependencies.LLVM.ValidationFile
    if (Test-Path $llvmConfig) {
        Write-Success "LLVM: LLVMConfig.cmake found"
    }
    else {
        Write-Error "LLVM: LLVMConfig.cmake NOT found at $llvmConfig"
        $allValid = $false
    }
    
    # ICU validation (check all required libs)
    # ICU uses different directory names for ARM64 vs x64
    if ($Arch -eq "arm64") {
        $icuLibSubdir = "libARM64"
        $icuBinSubdir = "binARM64"
    } else {
        $icuLibSubdir = "lib64"
        $icuBinSubdir = "bin64"
    }
    
    $icuLibs = @("icuuc.lib", "icuin.lib", "icudt.lib")
    $icuLibDir = Join-Path $Dependencies.ICU.TargetDir $icuLibSubdir
    $icuValid = $true
    foreach ($lib in $icuLibs) {
        $libPath = Join-Path $icuLibDir $lib
        if (-not (Test-Path $libPath)) {
            Write-Error "ICU: $lib NOT found at $libPath"
            $icuValid = $false
            $allValid = $false
        }
    }
    if ($icuValid) {
        Write-Success "ICU: All required libraries found (icuuc.lib, icuin.lib, icudt.lib)"
    }
    
    # ICU headers
    $icuHeader = Join-Path $Dependencies.ICU.TargetDir "include\unicode\utypes.h"
    if (Test-Path $icuHeader) {
        Write-Success "ICU: Headers found"
    }
    else {
        Write-Error "ICU: Headers NOT found at $icuHeader"
        $allValid = $false
    }
    
    # ICU DLLs
    $icuDlls = @("icuuc72.dll", "icuin72.dll", "icudt72.dll")
    $icuBinDir = Join-Path $Dependencies.ICU.TargetDir $icuBinSubdir
    $dllsValid = $true
    foreach ($dll in $icuDlls) {
        $dllPath = Join-Path $icuBinDir $dll
        if (-not (Test-Path $dllPath)) {
            Write-Error "ICU: $dll NOT found at $dllPath"
            $dllsValid = $false
            $allValid = $false
        }
    }
    if ($dllsValid) {
        Write-Success "ICU: All DLLs found (icuuc72.dll, icuin72.dll, icudt72.dll)"
    }
    
    # tomlplusplus validation
    $tomlHeader = Join-Path $Dependencies.TomlPlusPlus.TargetDir $Dependencies.TomlPlusPlus.ValidationFile
    if (Test-Path $tomlHeader) {
        Write-Success "tomlplusplus: Header found"
    }
    else {
        Write-Error "tomlplusplus: Header NOT found at $tomlHeader"
        $allValid = $false
    }
    
    return $allValid
}

# Main execution
Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Cursive 0 Compiler - Third Party Setup" -ForegroundColor Cyan
Write-Host "  Target Architecture: $Arch" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Check for 7-Zip (required for LLVM .tar.xz)
$7z = Get-7ZipPath
if (-not $7z) {
    Write-Warning "7-Zip not found. Required for extracting LLVM .tar.xz archives."
    Write-Host "  Please install 7-Zip from https://www.7-zip.org/" -ForegroundColor Yellow
    Write-Host ""
}

# Create third_party directory
if (-not (Test-Path $ThirdPartyDir)) {
    New-Item -ItemType Directory -Path $ThirdPartyDir -Force | Out-Null
    Write-Status "Created third_party directory"
}

# Download and extract dependencies
$success = $true

foreach ($name in @("LLVM", "ICU", "TomlPlusPlus")) {
    Write-Host ""
    if (-not (Get-Dependency -Name $name -Config $Dependencies[$name])) {
        $success = $false
    }
}

# Clean up temp directory
if (Test-Path $TempDir) {
    Remove-Item $TempDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Verify installation
Write-Host ""
$verified = Test-Dependencies

Write-Host ""
Write-Host "=========================================" -ForegroundColor Cyan

if ($success -and $verified) {
    Write-Host "  Setup completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "  You can now build the Cursive 0 compiler:" -ForegroundColor White
    Write-Host "    cd cursive-bootstrap" -ForegroundColor Gray
    Write-Host "    cmake -B build -G `"Visual Studio 17 2022`"" -ForegroundColor Gray
    Write-Host "    cmake --build build --config Release" -ForegroundColor Gray
}
else {
    Write-Host "  Setup completed with errors!" -ForegroundColor Red
    Write-Host "  Please check the messages above and retry." -ForegroundColor Yellow
    exit 1
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
