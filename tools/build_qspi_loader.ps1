param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$SourceDir = Join-Path $Root "tools\external_loader\ATK-DNH750_QSPI_W25Q64JV"
$BuildDir = Join-Path $Root "build\qspi_loader\$Config"
$ToolchainFile = Join-Path $Root "cmake\gcc-arm-none-eabi.cmake"
$LoaderElf = Join-Path $BuildDir "ATK-DNH750_QSPI_W25Q64JV.elf"
$LoaderOut = Join-Path $Root "tools\loaders\ATK-DNH750_QSPI_W25Q64JV.stldr"

function Resolve-CMake {
    $bundleRoot = Join-Path $env:LOCALAPPDATA "stm32cube\bundles\cmake"
    if (Test-Path -LiteralPath $bundleRoot) {
        $candidate = Get-ChildItem -LiteralPath $bundleRoot -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "bin\cmake.exe" } |
            Where-Object { Test-Path -LiteralPath $_ } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    $fromPath = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    throw "cmake.exe was not found. Install STM32Cube bundled CMake or add cmake to PATH."
}

function Resolve-Ninja {
    $bundleRoot = Join-Path $env:LOCALAPPDATA "stm32cube\bundles\ninja"
    if (Test-Path -LiteralPath $bundleRoot) {
        $candidate = Get-ChildItem -LiteralPath $bundleRoot -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "bin\ninja.exe" } |
            Where-Object { Test-Path -LiteralPath $_ } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    $fromPath = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    throw "ninja.exe was not found. Install STM32Cube bundled Ninja or add ninja to PATH."
}

$CMake = Resolve-CMake
$Ninja = Resolve-Ninja

$configureArgs = @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_MAKE_PROGRAM=$Ninja",
    "-DCMAKE_BUILD_TYPE=$Config"
)

if (-not (Test-Path -LiteralPath (Join-Path $BuildDir "CMakeCache.txt"))) {
    $configureArgs += "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile"
}

& $CMake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE"
}

& $CMake --build $BuildDir
if ($LASTEXITCODE -ne 0) {
    throw "QSPI external loader build failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path -LiteralPath $LoaderElf)) {
    throw "Expected loader ELF was not produced: $LoaderElf"
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LoaderOut) | Out-Null

if (Test-Path -LiteralPath $LoaderOut) {
    $sourceHash = (Get-FileHash -LiteralPath $LoaderElf -Algorithm SHA256).Hash
    $targetHash = (Get-FileHash -LiteralPath $LoaderOut -Algorithm SHA256).Hash
    if ($sourceHash -eq $targetHash) {
        Write-Host "QSPI external loader already up to date at $LoaderOut"
        exit 0
    }
}

Copy-Item -LiteralPath $LoaderElf -Destination $LoaderOut -Force

Write-Host "QSPI external loader written to $LoaderOut"
