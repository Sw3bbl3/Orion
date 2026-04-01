[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [string]$DistDir = "dist",
    [switch]$SkipBuild,
    [switch]$SkipZip,
    [switch]$SkipInstaller,
    [switch]$SkipCodeSign,
    [string]$SignCertificateFile = $env:ORION_SIGN_CERT_FILE,
    [string]$SignCertificatePassword = $env:ORION_SIGN_CERT_PASSWORD,
    [string]$SignThumbprint = $env:ORION_SIGN_CERT_SHA1,
    [string]$SignSubjectName = $env:ORION_SIGN_CERT_SUBJECT,
    [switch]$SignUseMachineStore,
    [string]$SignTimestampUrl = $(if ([string]::IsNullOrWhiteSpace($env:ORION_SIGN_TIMESTAMP_URL)) { "http://timestamp.digicert.com" } else { $env:ORION_SIGN_TIMESTAMP_URL }),
    [string]$SignDescription = "Orion"
)

$ErrorActionPreference = "Stop"

function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$Action
    )

    Write-Host ""
    Write-Host "==> $Name"
    & $Action
}

function Require-Command {
    param([Parameter(Mandatory = $true)][string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found in PATH."
    }
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Exe,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $Exe $($Arguments -join ' ')"
    }
}

function Find-SignTool {
    $cmd = Get-Command "signtool.exe" -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $kitsBase = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
    if (-not (Test-Path $kitsBase)) {
        return $null
    }

    $candidates = Get-ChildItem -Path $kitsBase -Directory -ErrorAction SilentlyContinue |
        Sort-Object -Property Name -Descending |
        ForEach-Object { Join-Path $_.FullName "x64\signtool.exe" } |
        Where-Object { Test-Path $_ }

    return ($candidates | Select-Object -First 1)
}

function Invoke-CodeSign {
    param(
        [Parameter(Mandatory = $true)][string]$SignToolPath,
        [Parameter(Mandatory = $true)][string]$FilePath
    )

    if (-not (Test-Path $FilePath)) {
        throw "Cannot sign missing file: $FilePath"
    }

    $signArgs = @(
        "sign",
        "/fd", "SHA256",
        "/td", "SHA256",
        "/tr", $SignTimestampUrl,
        "/d", $SignDescription
    )

    if (-not [string]::IsNullOrWhiteSpace($SignCertificateFile)) {
        if (-not (Test-Path $SignCertificateFile)) {
            throw "Signing certificate file was not found: $SignCertificateFile"
        }

        $signArgs += @("/f", $SignCertificateFile)
        if (-not [string]::IsNullOrWhiteSpace($SignCertificatePassword)) {
            $signArgs += @("/p", $SignCertificatePassword)
        }
    } else {
        if ([string]::IsNullOrWhiteSpace($SignThumbprint)) {
            throw "No signing identity was provided."
        }

        $signArgs += @("/sha1", $SignThumbprint)
        if (-not [string]::IsNullOrWhiteSpace($SignSubjectName)) {
            $signArgs += @("/n", $SignSubjectName)
        }
        if ($SignUseMachineStore) {
            $signArgs += "/sm"
        }
    }

    $signArgs += @("/v", $FilePath)
    Invoke-Checked -Exe $SignToolPath -Arguments $signArgs
}

Require-Command "cmake"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $repoRoot $BuildDir
$distPath = Join-Path $repoRoot $DistDir
$installPath = Join-Path $distPath "Orion"
$installerOutputPath = Join-Path $distPath "installer"
$installerArtPath = Join-Path $repoRoot "installer\art"
$installerScriptPath = Join-Path $repoRoot "installer\OrionInstaller.iss"
$installerArtGeneratorPath = Join-Path $repoRoot "installer\prepare-installer-art.ps1"
$setupIconPath = Join-Path $repoRoot "assets\orion\Orion_logo.ico"

$isCodeSigningConfigured = (-not $SkipCodeSign) -and (
    -not [string]::IsNullOrWhiteSpace($SignCertificateFile) -or
    -not [string]::IsNullOrWhiteSpace($SignThumbprint)
)

$signToolPath = $null
if ($isCodeSigningConfigured) {
    $signToolPath = Find-SignTool
    if (-not $signToolPath) {
        throw "Code signing is configured, but signtool.exe was not found. Install Windows SDK Signing Tools."
    }
}

Invoke-Step -Name "Configure + Build" -Action {
    if ($SkipBuild) {
        Write-Host "Skipping build because -SkipBuild was passed."
        return
    }

    if (-not (Test-Path $buildPath)) {
        New-Item -Path $buildPath -ItemType Directory -Force | Out-Null
    }

    $cachePath = Join-Path $buildPath "CMakeCache.txt"
    $configureArgs = @(
        "-S", $repoRoot,
        "-B", $buildPath,
        "-G", "Visual Studio 17 2022",
        "-A", "x64"
    )

    if (Test-Path $cachePath) {
        Write-Host "Re-configuring existing build directory."
    } else {
        Write-Host "Configuring new build directory."
    }

    Invoke-Checked -Exe "cmake" -Arguments $configureArgs

    Invoke-Checked -Exe "cmake" -Arguments @(
        "--build", $buildPath,
        "--config", $Config
    )
}

Invoke-Step -Name "Stage install layout" -Action {
    if (Test-Path $installPath) {
        Remove-Item -Path $installPath -Recurse -Force
    }
    New-Item -Path $installPath -ItemType Directory -Force | Out-Null

    Invoke-Checked -Exe "cmake" -Arguments @(
        "--install", $buildPath,
        "--config", $Config,
        "--component", "OrionRuntime",
        "--prefix", $installPath
    )

    $exePath = Join-Path $installPath "Orion.exe"
    $assetsPath = Join-Path $installPath "assets"
    if (-not (Test-Path $exePath)) {
        throw "Packaging failed: '$exePath' was not created."
    }
    if (-not (Test-Path $assetsPath)) {
        throw "Packaging failed: '$assetsPath' was not created."
    }
}

$exeForVersion = Join-Path $installPath "Orion.exe"
$rawVersion = (Get-Item $exeForVersion).VersionInfo.ProductVersion
if ([string]::IsNullOrWhiteSpace($rawVersion)) {
    $rawVersion = (Get-Item $exeForVersion).VersionInfo.FileVersion
}
if ([string]::IsNullOrWhiteSpace($rawVersion)) {
    $rawVersion = "0.1.0"
}
$version = ($rawVersion -replace "[^0-9\.]", "")
if ([string]::IsNullOrWhiteSpace($version)) {
    $version = "0.1.0"
}

if ($isCodeSigningConfigured) {
    Invoke-Step -Name "Code sign Orion executable" -Action {
        Invoke-CodeSign -SignToolPath $signToolPath -FilePath $exeForVersion
    }
} else {
    Write-Host ""
    Write-Host "==> Code signing"
    if ($SkipCodeSign) {
        Write-Host "Code signing skipped by flag."
    } else {
        Write-Host "Code signing not configured. Set ORION_SIGN_CERT_FILE (or ORION_SIGN_CERT_SHA1) to enable."
    }
}

if (-not $SkipZip) {
    Invoke-Step -Name "Create portable ZIP" -Action {
        if (-not (Test-Path $distPath)) {
            New-Item -Path $distPath -ItemType Directory -Force | Out-Null
        }

        $zipPath = Join-Path $distPath ("Orion-{0}-windows-x64-portable.zip" -f $version)
        if (Test-Path $zipPath) {
            Remove-Item -Path $zipPath -Force
        }

        Compress-Archive -Path (Join-Path $installPath "*") -DestinationPath $zipPath -CompressionLevel Optimal
        Write-Host "Portable package: $zipPath"
    }
}

Invoke-Step -Name "Generate installer artwork" -Action {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $installerArtGeneratorPath -OutputDir $installerArtPath
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to generate installer artwork."
    }
}

if (-not $SkipInstaller) {
    Invoke-Step -Name "Build installer EXE (Inno Setup)" -Action {
        $iscc = Get-Command "iscc" -ErrorAction SilentlyContinue
        if (-not $iscc) {
            Write-Warning "Inno Setup compiler (iscc.exe) not found. Install Inno Setup and rerun to generate the installer EXE."
            return
        }

        if (-not (Test-Path $installerOutputPath)) {
            New-Item -Path $installerOutputPath -ItemType Directory -Force | Out-Null
        }

        $issArgs = @(
            "/Qp",
            "/DAppBuildDir=$installPath",
            "/DInstallerOutputDir=$installerOutputPath",
            "/DWizardAssetsDir=$installerArtPath",
            "/DSetupIconPath=$setupIconPath",
            $installerScriptPath
        )

        Invoke-Checked -Exe $iscc.Source -Arguments $issArgs

        if ($isCodeSigningConfigured) {
            $installerExe = Get-ChildItem -Path $installerOutputPath -Filter "Orion-Setup-*.exe" -File |
                Sort-Object LastWriteTime -Descending |
                Select-Object -First 1

            if (-not $installerExe) {
                throw "Installer build completed but no setup executable was found in '$installerOutputPath'."
            }

            Invoke-CodeSign -SignToolPath $signToolPath -FilePath $installerExe.FullName
            Write-Host "Signed installer: $($installerExe.FullName)"
        }
    }
}

Write-Host ""
Write-Host "Packaging complete."
Write-Host "Staged app directory: $installPath"
Write-Host "Installer output dir: $installerOutputPath"
