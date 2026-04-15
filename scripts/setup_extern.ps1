Param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$ExternDir = "",
    [string]$IcuTag = "release-72-1",
    [string]$IcuAssetPattern = "Win64.*MSVC.*\.zip",
    [string]$LlvmTag = "llvmorg-21.1.8",
    [string]$LlvmAssetPattern = "clang\+llvm-.*x86_64-.*windows-msvc.*\.(tar\.xz|zip)$",
    [string]$TomlTag = "v3.4.0",
    [switch]$Force,
    [switch]$NoCache
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ([string]::IsNullOrWhiteSpace($ExternDir)) {
    $ExternDir = Join-Path $RepoRoot "extern"
}

function Write-Info([string]$Message) {
    Write-Host "[extern] $Message"
}

function Write-Warn([string]$Message) {
    Write-Warning "[extern] $Message"
}

function Invoke-GitHubApi([string]$Uri) {
    $headers = @{
        "User-Agent" = "Cursive-extern-setup"
        "Accept"     = "application/vnd.github+json"
    }
    if ($env:GITHUB_TOKEN) {
        $headers["Authorization"] = "Bearer $env:GITHUB_TOKEN"
    }
    return Invoke-RestMethod -Uri $Uri -Headers $headers
}

function Ensure-EmptyDir([string]$Path, [switch]$ForceRemove) {
    if (Test-Path $Path) {
        if ($ForceRemove) {
            Remove-Item -Path $Path -Recurse -Force
        } else {
            return $false
        }
    }
    New-Item -ItemType Directory -Force $Path | Out-Null
    return $true
}

function Download-File([string]$Url, [string]$OutFile, [Nullable[Int64]]$ExpectedSize = $null) {
    if (Test-Path $OutFile) {
        if (-not $NoCache) {
            if ($ExpectedSize -eq $null) {
                Write-Info "Using cached download: $OutFile"
                return
            }
            $currentSize = (Get-Item $OutFile).Length
            if ($currentSize -eq $ExpectedSize) {
                Write-Info "Using cached download: $OutFile"
                return
            }
            if ($currentSize -lt $ExpectedSize) {
                $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
                if ($curl) {
                    Write-Warn "Cached download incomplete for $OutFile (have $currentSize, expected $ExpectedSize). Resuming via curl."
                    & $curl.Path -L --retry 3 --continue-at - -o $OutFile $Url
                    $currentSize = (Get-Item $OutFile).Length
                    if ($currentSize -eq $ExpectedSize) {
                        Write-Info "Resumed download complete: $OutFile"
                        return
                    }
                    Write-Warn "Resume did not complete download (have $currentSize, expected $ExpectedSize). Re-downloading."
                } else {
                    Write-Warn "Cached download incomplete for $OutFile (have $currentSize, expected $ExpectedSize). Re-downloading."
                }
            } else {
                Write-Warn "Cached download size mismatch for $OutFile (have $currentSize, expected $ExpectedSize). Re-downloading."
            }
        }
        Remove-Item -Force $OutFile -ErrorAction SilentlyContinue
    }
    Write-Info "Downloading: $Url"
    Invoke-WebRequest -Uri $Url -OutFile $OutFile
    if ($ExpectedSize -ne $null) {
        $finalSize = (Get-Item $OutFile).Length
        if ($finalSize -ne $ExpectedSize) {
            throw "Download size mismatch for $OutFile (have $finalSize, expected $ExpectedSize)."
        }
    }
}

function Expand-Zip([string]$Archive, [string]$Destination) {
    Expand-Archive -Path $Archive -DestinationPath $Destination -Force
}

function Expand-TarXz([string]$Archive, [string]$Destination) {
    $tar = Get-Command tar -ErrorAction SilentlyContinue
    if (-not $tar) {
        throw "tar is required to extract $Archive (not found in PATH)."
    }
    Push-Location $Destination
    try {
        & $tar.Path -xf $Archive
    } finally {
        Pop-Location
    }
}

function Find-DirWithChildren([string]$Root, [string[]]$RequiredRelativePaths) {
    if ($RequiredRelativePaths | ForEach-Object { Test-Path (Join-Path $Root $_) } | Where-Object { -not $_ }) {
        $match = Get-ChildItem -Path $Root -Directory -Recurse | Where-Object {
            $dir = $_.FullName
            $missing = $false
            foreach ($rel in $RequiredRelativePaths) {
                if (-not (Test-Path (Join-Path $dir $rel))) {
                    $missing = $true
                    break
                }
            }
            -not $missing
        } | Select-Object -First 1
        return $match
    }
    return Get-Item -Path $Root
}

function Copy-DirContents([string]$SourceDir, [string]$TargetDir) {
    if (-not (Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Force $TargetDir | Out-Null
    }
    Copy-Item -Path (Join-Path $SourceDir "*") -Destination $TargetDir -Recurse -Force
}

function Copy-RequiredDir([string]$SourceDir, [string]$TargetDir) {
    if (-not (Test-Path $SourceDir)) {
        throw "Missing required directory: $SourceDir"
    }
    if (Test-Path $TargetDir) {
        Remove-Item -Path $TargetDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force (Split-Path -Parent $TargetDir) | Out-Null
    Copy-Item -Path $SourceDir -Destination $TargetDir -Recurse -Force
}

function Copy-RequiredFile([string]$SourceFile, [string]$TargetFile) {
    if (-not (Test-Path $SourceFile)) {
        throw "Missing required file: $SourceFile"
    }
    New-Item -ItemType Directory -Force (Split-Path -Parent $TargetFile) | Out-Null
    Copy-Item -Path $SourceFile -Destination $TargetFile -Force
}

function Find-CachedDownload([string]$Directory, [string]$Pattern) {
    if (-not (Test-Path $Directory)) {
        return $null
    }
    return Get-ChildItem -Path $Directory -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match $Pattern } |
        Sort-Object Name |
        Select-Object -First 1
}

function Patch-LlvmImportChecks([string]$LlvmRoot) {
    $exports = Join-Path $LlvmRoot "lib\\cmake\\llvm\\LLVMExports.cmake"
    if (-not (Test-Path $exports)) {
        Write-Warn "LLVMExports.cmake not found at $exports; skipping import-check patch."
        return
    }

    $content = Get-Content -Path $exports -Raw
    $patched = $content.Replace(
        "# Loop over all imported files and verify that they actually exist`r`nforeach(_cmake_target IN LISTS _cmake_import_check_targets)`r`n",
        "# Vendored Cursive extern keeps only the build-consumed LLVM subset.`r`nset(_cmake_import_check_targets)`r`n# Loop over all imported files and verify that they actually exist`r`nforeach(_cmake_target IN LISTS _cmake_import_check_targets)`r`n"
    )
    $patched = $patched.Replace(
        "# Loop over all imported files and verify that they actually exist`r`nforeach(target `${_IMPORT_CHECK_TARGETS} )`r`n",
        "# Vendored Cursive extern keeps only the build-consumed LLVM subset.`r`nset(_IMPORT_CHECK_TARGETS)`r`n# Loop over all imported files and verify that they actually exist`r`nforeach(target `${_IMPORT_CHECK_TARGETS} )`r`n"
    )

    if ($patched -ne $content) {
        Set-Content -Path $exports -Value $patched -Encoding UTF8
        Write-Info "Patched LLVM import checks for pruned vendored extern."
    }
}

function Setup-Icu {
    $icuTarget = Join-Path $ExternDir "icu\\win64"
    if (-not (Ensure-EmptyDir $icuTarget -ForceRemove:$Force)) {
        Write-Info "ICU already present at $icuTarget (use -Force to replace)."
        return
    }

    $downloadDir = Join-Path $ExternDir ".downloads"
    New-Item -ItemType Directory -Force $downloadDir | Out-Null
    $cachedAsset = $null
    if (-not $NoCache) {
        $cachedAsset = Find-CachedDownload $downloadDir $IcuAssetPattern
    }

    if ($cachedAsset) {
        $archivePath = $cachedAsset.FullName
        Write-Info "Using cached ICU archive: $archivePath"
    } else {
        $release = Invoke-GitHubApi "https://api.github.com/repos/unicode-org/icu/releases/tags/$IcuTag"
        $assets = $release.assets | Where-Object { $_.name -match $IcuAssetPattern }
        if (-not $assets) {
            throw "ICU asset not found for tag $IcuTag (pattern: $IcuAssetPattern)."
        }

        $asset = ($assets | Where-Object { $_.name -match "MSVC2022" } | Select-Object -First 1)
        if (-not $asset) {
            $asset = ($assets | Where-Object { $_.name -match "MSVC2019" } | Select-Object -First 1)
        }
        if (-not $asset) {
            $asset = $assets | Select-Object -First 1
        }

        $archivePath = Join-Path $downloadDir $asset.name
        Download-File $asset.browser_download_url $archivePath $asset.size
    }

    $tempDir = Join-Path $ExternDir ".tmp_icu"
    if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
    New-Item -ItemType Directory -Force $tempDir | Out-Null
    Expand-Zip $archivePath $tempDir

    $icuRoot = Find-DirWithChildren $tempDir @("include", "lib64", "bin64")
    if (-not $icuRoot) {
        $nestedZip = Get-ChildItem -Path $tempDir -Recurse -Filter "*.zip" | Select-Object -First 1
        if ($nestedZip) {
            $nestedDir = Join-Path $tempDir "_nested"
            if (Test-Path $nestedDir) { Remove-Item -Recurse -Force $nestedDir }
            New-Item -ItemType Directory -Force $nestedDir | Out-Null
            Expand-Zip $nestedZip.FullName $nestedDir
            $icuRoot = Find-DirWithChildren $nestedDir @("include", "lib64", "bin64")
        }
    }
    if (-not $icuRoot) {
        throw "Failed to locate ICU root with include/lib64/bin64 in $tempDir."
    }

    Copy-RequiredDir (Join-Path $icuRoot.FullName "include") (Join-Path $icuTarget "include")
    New-Item -ItemType Directory -Force (Join-Path $icuTarget "lib64") | Out-Null
    New-Item -ItemType Directory -Force (Join-Path $icuTarget "bin64") | Out-Null
    Copy-RequiredFile (Join-Path $icuRoot.FullName "lib64\\icuuc.lib") (Join-Path $icuTarget "lib64\\icuuc.lib")
    Copy-RequiredFile (Join-Path $icuRoot.FullName "lib64\\icuin.lib") (Join-Path $icuTarget "lib64\\icuin.lib")
    Copy-RequiredFile (Join-Path $icuRoot.FullName "lib64\\icudt.lib") (Join-Path $icuTarget "lib64\\icudt.lib")
    Copy-RequiredFile (Join-Path $icuRoot.FullName "bin64\\icuuc72.dll") (Join-Path $icuTarget "bin64\\icuuc72.dll")
    Copy-RequiredFile (Join-Path $icuRoot.FullName "bin64\\icuin72.dll") (Join-Path $icuTarget "bin64\\icuin72.dll")
    Copy-RequiredFile (Join-Path $icuRoot.FullName "bin64\\icudt72.dll") (Join-Path $icuTarget "bin64\\icudt72.dll")
    Remove-Item -Recurse -Force $tempDir
    Write-Info "ICU installed at $icuTarget"
}

function Setup-Toml {
    $tomlTarget = Join-Path $ExternDir "tomlplusplus\\include"
    if (-not (Ensure-EmptyDir $tomlTarget -ForceRemove:$Force)) {
        Write-Info "toml++ already present at $tomlTarget (use -Force to replace)."
        return
    }

    if ($TomlTag -eq "latest") {
        $release = Invoke-GitHubApi "https://api.github.com/repos/marzer/tomlplusplus/releases/latest"
        $zipUrl = $release.zipball_url
        $tagName = $release.tag_name
    } else {
        $zipUrl = "https://github.com/marzer/tomlplusplus/archive/refs/tags/$TomlTag.zip"
        $tagName = $TomlTag
    }

    $downloadDir = Join-Path $ExternDir ".downloads"
    New-Item -ItemType Directory -Force $downloadDir | Out-Null
    $archivePath = Join-Path $downloadDir "tomlplusplus-$tagName.zip"
    Download-File $zipUrl $archivePath

    $tempDir = Join-Path $ExternDir ".tmp_toml"
    if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
    New-Item -ItemType Directory -Force $tempDir | Out-Null
    Expand-Zip $archivePath $tempDir

    $tomlRoot = Find-DirWithChildren $tempDir @("include", "include\\toml++\\toml.hpp")
    if (-not $tomlRoot) {
        throw "Failed to locate toml++ include directory in $tempDir."
    }

    Copy-DirContents (Join-Path $tomlRoot.FullName "include") $tomlTarget
    Remove-Item -Recurse -Force $tempDir
    Write-Info "toml++ installed at $tomlTarget"
}

function Setup-Llvm {
    $llvmVersion = $LlvmTag -replace "^llvmorg-", ""
    $llvmRootName = "llvm-$llvmVersion-x86_64"
    $llvmTarget = Join-Path $ExternDir "llvm\\$llvmRootName"

    if (-not (Ensure-EmptyDir $llvmTarget -ForceRemove:$Force)) {
        Write-Info "LLVM already present at $llvmTarget (use -Force to replace)."
        Patch-LlvmDia $llvmTarget
        return
    }

    $downloadDir = Join-Path $ExternDir ".downloads"
    New-Item -ItemType Directory -Force $downloadDir | Out-Null
    $cachedAsset = $null
    if (-not $NoCache) {
        $cachedAsset = Find-CachedDownload $downloadDir $LlvmAssetPattern
    }

    if ($cachedAsset) {
        $archivePath = $cachedAsset.FullName
        $assetName = $cachedAsset.Name
        Write-Info "Using cached LLVM archive: $archivePath"
    } else {
        $release = Invoke-GitHubApi "https://api.github.com/repos/llvm/llvm-project/releases/tags/$LlvmTag"
        $assets = $release.assets | Where-Object { $_.name -match $LlvmAssetPattern }
        if (-not $assets) {
            throw "LLVM asset not found for tag $LlvmTag (pattern: $LlvmAssetPattern)."
        }

        $asset = ($assets | Where-Object { $_.name -match "\.tar\.xz$" } | Select-Object -First 1)
        if (-not $asset) {
            $asset = $assets | Select-Object -First 1
        }

        $assetName = $asset.name
        $archivePath = Join-Path $downloadDir $assetName
        Download-File $asset.browser_download_url $archivePath $asset.size
    }

    $tempDir = Join-Path $ExternDir ".tmp_llvm"
    if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
    New-Item -ItemType Directory -Force $tempDir | Out-Null

    if ($assetName -match "\.tar\.xz$") {
        Expand-TarXz $archivePath $tempDir
    } else {
        Expand-Zip $archivePath $tempDir
    }

    $llvmRoot = Find-DirWithChildren $tempDir @("include", "lib", "lib\\cmake\\llvm")
    if (-not $llvmRoot) {
        throw "Failed to locate LLVM root with lib/cmake/llvm in $tempDir."
    }

    New-Item -ItemType Directory -Force (Join-Path $llvmTarget "include") | Out-Null
    New-Item -ItemType Directory -Force (Join-Path $llvmTarget "lib\\cmake") | Out-Null
    Copy-RequiredDir (Join-Path $llvmRoot.FullName "include\\llvm") (Join-Path $llvmTarget "include\\llvm")
    Copy-RequiredDir (Join-Path $llvmRoot.FullName "include\\llvm-c") (Join-Path $llvmTarget "include\\llvm-c")
    Copy-RequiredDir (Join-Path $llvmRoot.FullName "lib\\cmake\\llvm") (Join-Path $llvmTarget "lib\\cmake\\llvm")
    New-Item -ItemType Directory -Force (Join-Path $llvmTarget "lib") | Out-Null
    Get-ChildItem -Path (Join-Path $llvmRoot.FullName "lib") -File |
        Where-Object { $_.Name -like "LLVM*.lib" } |
        ForEach-Object {
            Copy-RequiredFile $_.FullName (Join-Path $llvmTarget "lib\\$($_.Name)")
        }
    Copy-RequiredFile (Join-Path $llvmRoot.FullName "bin\\lld-link.exe") (Join-Path $llvmTarget "bin\\lld-link.exe")
    Copy-RequiredFile (Join-Path $llvmRoot.FullName "bin\\llvm-ar.exe") (Join-Path $llvmTarget "bin\\llvm-ar.exe")
    Copy-RequiredFile (Join-Path $llvmRoot.FullName "bin\\llvm-lib.exe") (Join-Path $llvmTarget "bin\\llvm-lib.exe")
    Copy-RequiredFile (Join-Path $llvmRoot.FullName "bin\\llvm-as.exe") (Join-Path $llvmTarget "bin\\llvm-as.exe")
    if (-not (Test-Path (Join-Path $llvmTarget "lib\\LLVMCore.lib"))) {
        throw "Failed to stage LLVM libraries into $llvmTarget\\lib."
    }
    Remove-Item -Recurse -Force $tempDir
    Write-Info "LLVM installed at $llvmTarget"
    Patch-LlvmImportChecks $llvmTarget
    Patch-LlvmDia $llvmTarget
}

function Find-DiaLib {
    $roots = @(
        "C:\\Program Files\\Microsoft Visual Studio",
        "C:\\Program Files (x86)\\Microsoft Visual Studio"
    )
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) {
            continue
        }
        $found = Get-ChildItem -Path $root -Recurse -Filter diaguids.lib -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "DIA SDK\\\\lib\\\\amd64\\\\diaguids\\.lib$" } |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }
    return $null
}

function Patch-LlvmDia([string]$LlvmRoot) {
    $exports = Join-Path $LlvmRoot "lib\\cmake\\llvm\\LLVMExports.cmake"
    if (-not (Test-Path $exports)) {
        Write-Warn "LLVMExports.cmake not found at $exports; skipping DIA patch."
        return
    }
    $content = Get-Content -Path $exports -Raw
    if ($content -notmatch "diaguids\\.lib") {
        Write-Info "LLVMExports.cmake does not reference diaguids.lib; skipping patch."
        return
    }
    $patched = [regex]::Replace(
        $content,
        "[A-Za-z]:/Program Files(?: \\(x86\\))?/Microsoft Visual Studio/[^;\""]+/DIA SDK/lib/amd64/diaguids\\.lib",
        "diaguids.lib"
    )
    if ($patched -ne $content) {
        Set-Content -Path $exports -Value $patched -Encoding UTF8
        $diaLib = Find-DiaLib
        if ($diaLib) {
            Write-Info "Patched LLVM DIA link to linker-resolved diaguids.lib (verified at $diaLib)"
        } else {
            Write-Warn "Patched LLVM DIA link to linker-resolved diaguids.lib, but no local DIA SDK was detected."
        }
    } else {
        Write-Info "LLVMExports.cmake already uses a portable DIA reference."
    }
}

Write-Info "Repo root: $RepoRoot"
Write-Info "Extern dir: $ExternDir"

New-Item -ItemType Directory -Force $ExternDir | Out-Null
Setup-Icu
Setup-Toml
Setup-Llvm

Write-Info "Extern setup complete."
