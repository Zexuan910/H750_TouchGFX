param(
    [string]$LoaderPath = "tools\loaders\ATK-DNH750_QSPI_W25Q64JV.stldr"
)

$ErrorActionPreference = "Stop"

function Resolve-ArmTool {
    param([string]$Name)

    $bundleRoot = Join-Path $env:LOCALAPPDATA "stm32cube\bundles\gnu-tools-for-stm32"
    if (Test-Path -LiteralPath $bundleRoot) {
        $candidate = Get-ChildItem -LiteralPath $bundleRoot -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "bin\$Name" } |
            Where-Object { Test-Path -LiteralPath $_ } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    $fromPath = Get-Command $Name -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    throw "$Name was not found. Install STM32Cube bundled GNU tools or add arm-none-eabi-* to PATH."
}

function Invoke-Tool {
    param(
        [string]$Tool,
        [string[]]$Arguments
    )

    $output = & $Tool @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "$Tool failed with exit code $LASTEXITCODE`n$output"
    }
    return $output
}

function Read-UInt32LE {
    param(
        [byte[]]$Bytes,
        [int]$Offset
    )

    return [uint32](
        [uint32]$Bytes[$Offset] -bor
        ([uint32]$Bytes[$Offset + 1] -shl 8) -bor
        ([uint32]$Bytes[$Offset + 2] -shl 16) -bor
        ([uint32]$Bytes[$Offset + 3] -shl 24)
    )
}

if ([System.IO.Path]::GetExtension($LoaderPath) -ine ".stldr") {
    throw "LoaderPath must point to a STM32CubeProgrammer .stldr file: $LoaderPath"
}

if (-not (Test-Path -LiteralPath $LoaderPath)) {
    throw "External loader not found: $LoaderPath"
}

$LoaderPath = (Resolve-Path -LiteralPath $LoaderPath).Path
$nm = Resolve-ArmTool "arm-none-eabi-nm.exe"
$readelf = Resolve-ArmTool "arm-none-eabi-readelf.exe"
$objcopy = Resolve-ArmTool "arm-none-eabi-objcopy.exe"

$nmOutput = Invoke-Tool $nm @($LoaderPath)
$readelfOutput = Invoke-Tool $readelf @("-S", $LoaderPath)
$readelfProgramOutput = Invoke-Tool $readelf @("-l", $LoaderPath)

$requiredSymbols = @(
    "StorageInfo",
    "Init",
    "Read",
    "Write",
    "SectorErase",
    "MassErase",
    "Verify",
    "CheckSum"
)

foreach ($symbol in $requiredSymbols) {
    $pattern = "^[0-9a-fA-F]{8}\s+[A-Za-z]\s+$symbol$"
    if (-not ($nmOutput | Where-Object { $_ -match $pattern })) {
        throw "Missing required external-loader symbol: $symbol"
    }
}

$storageLine = $nmOutput | Where-Object { $_ -match "^[0-9a-fA-F]{8}\s+[A-Za-z]\s+StorageInfo$" } | Select-Object -First 1
$storageAddress = [Convert]::ToUInt32(($storageLine -split "\s+")[0], 16)
if ($storageAddress -ne 0x00000000) {
    throw ("StorageInfo must be linked at 0x00000000, got 0x{0:X8}" -f $storageAddress)
}

if (-not ($readelfOutput | Where-Object { $_ -match "\.Dev_Info\s+PROGBITS\s+00000000" })) {
    throw "Missing .Dev_Info section at 0x00000000"
}

if (-not ($readelfOutput | Where-Object { $_ -match "\s+PROGBITS\s+2000[0-9a-fA-F]{4}" })) {
    throw "No loadable code/rodata section was found in the 0x20000000 RAM loader window"
}

$ramLoadSegments = @()
foreach ($line in $readelfProgramOutput) {
    if ($line -match "^\s+LOAD\s+0x[0-9A-Fa-f]+\s+0x([0-9A-Fa-f]+)\s+0x[0-9A-Fa-f]+\s+0x[0-9A-Fa-f]+\s+0x[0-9A-Fa-f]+\s+(.+?)\s+0x[0-9A-Fa-f]+\s*$") {
        $vma = [Convert]::ToUInt32($Matches[1], 16)
        if (($vma -ge 0x20000000) -and ($vma -lt 0x20020000)) {
            $ramLoadSegments += [PSCustomObject]@{
                Line = $line
                Flags = ($Matches[2] -replace "\s", "")
            }
        }
    }
}

if ($ramLoadSegments.Count -ne 1) {
    throw ("External loader must use one contiguous RAM PT_LOAD segment; found {0}. CubeProgrammer can mis-handle split text/data loader images." -f $ramLoadSegments.Count)
}

if ($ramLoadSegments[0].Flags -ne "RWE") {
    throw ("External-loader RAM PT_LOAD segment must be RWE, got flags '{0}'." -f $ramLoadSegments[0].Flags)
}

$temp = [System.IO.Path]::GetTempFileName()
try {
    Invoke-Tool $objcopy @("-O", "binary", "-j", ".Dev_Info", $LoaderPath, $temp) | Out-Null
    [byte[]]$storageBytes = Get-Content -Encoding Byte -LiteralPath $temp
} finally {
    if (Test-Path -LiteralPath $temp) {
        Remove-Item -LiteralPath $temp -Force
    }
}

if ($storageBytes.Length -lt 128) {
    throw ".Dev_Info is too small to contain StorageInfo"
}

$deviceName = [System.Text.Encoding]::ASCII.GetString($storageBytes[0..99]).Trim([char]0)
$deviceType = Read-UInt32LE $storageBytes 100
$deviceStart = Read-UInt32LE $storageBytes 104
$deviceSize = Read-UInt32LE $storageBytes 108
$pageSize = Read-UInt32LE $storageBytes 112
$eraseValue = Read-UInt32LE $storageBytes 116
$sectorCount = Read-UInt32LE $storageBytes 120
$sectorSize = Read-UInt32LE $storageBytes 124

if ($deviceName -ne "ATK-DNH750_QSPI_W25Q64JV") {
    throw "Unexpected StorageInfo device name: $deviceName"
}
if ($deviceType -ne 3) {
    throw "Unexpected device type: $deviceType"
}
$expectedStart = [Convert]::ToUInt32("90000000", 16)
$expectedSize = [Convert]::ToUInt32("00800000", 16)
$expectedPageSize = [Convert]::ToUInt32("00000100", 16)
$expectedEraseValue = [Convert]::ToUInt32("000000FF", 16)
$expectedSectorSize = [Convert]::ToUInt32("00001000", 16)

if ($deviceStart -ne $expectedStart) {
    throw ("Unexpected device start address: 0x{0:X8}" -f $deviceStart)
}
if ($deviceSize -ne $expectedSize) {
    throw ("Unexpected device size: 0x{0:X8}" -f $deviceSize)
}
if ($pageSize -ne $expectedPageSize) {
    throw ("Unexpected write/page size: 0x{0:X8}" -f $pageSize)
}
if ($eraseValue -ne $expectedEraseValue) {
    throw ("Unexpected erase value: 0x{0:X8}" -f $eraseValue)
}
if (($sectorCount -ne 2048) -or ($sectorSize -ne $expectedSectorSize)) {
    throw ("Unexpected sector geometry: {0} x 0x{1:X}" -f $sectorCount, $sectorSize)
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$loaderHeader = Join-Path $repoRoot "tools\external_loader\ATK-DNH750_QSPI_W25Q64JV\Inc\loader_api.h"
if (Test-Path -LiteralPath $loaderHeader) {
    $loaderHeaderText = Get-Content -LiteralPath $loaderHeader -Raw
    if (-not $loaderHeaderText.Contains("#define LOADER_WRITE_BLOCK_SIZE  0x00000100U")) {
        throw "External-loader StorageInfo PageSize must match the W25Q page-program size: 0x100."
    }
}

$loaderApi = Join-Path $repoRoot "tools\external_loader\ATK-DNH750_QSPI_W25Q64JV\Src\loader_api.c"
if (Test-Path -LiteralPath $loaderApi) {
    $loaderApiText = Get-Content -LiteralPath $loaderApi -Raw
    if (-not $loaderApiText.Contains("erase_range_to_offsets") -or
        -not $loaderApiText.Contains("LOADER_SECTOR_COUNT") -or
        -not $loaderApiText.Contains("LOADER_SECTOR_SIZE")) {
        throw "SectorErase must accept both absolute addresses and legacy sector-index ranges."
    }
}

$qspiSource = Join-Path $repoRoot "Core\Src\qspi.c"
if (Test-Path -LiteralPath $qspiSource) {
    $qspiText = Get-Content -LiteralPath $qspiSource -Raw
    if (-not $qspiText.Contains("__HAL_RCC_QSPI_FORCE_RESET()") -or
        -not $qspiText.Contains("__HAL_RCC_QSPI_RELEASE_RESET()")) {
        throw "External-loader QSPI init must force-reset QUADSPI before reuse."
    }
}

$debugHeader = Join-Path $repoRoot "tools\external_loader\ATK-DNH750_QSPI_W25Q64JV\Inc\loader_debug.h"
if (Test-Path -LiteralPath $debugHeader) {
    $debugBaseLine = Get-Content -LiteralPath $debugHeader |
        Where-Object { $_ -match "^\s*#define\s+LOADER_DEBUG_BASE\s+0x([0-9A-Fa-f]+)U" } |
        Select-Object -First 1

    if ($debugBaseLine) {
        $debugBase = [Convert]::ToUInt32(([regex]::Match($debugBaseLine, "0x([0-9A-Fa-f]+)U").Groups[1].Value), 16)
        $ramStart = [Convert]::ToUInt32("20000000", 16)
        $ramEnd = [Convert]::ToUInt32("20020000", 16)
        $stackGuard = [Convert]::ToUInt32("00001000", 16)
        if (($debugBase -lt $ramStart) -or ($debugBase -ge ($ramEnd - $stackGuard))) {
            throw ("LOADER_DEBUG_BASE must stay out of the external-loader stack guard: 0x{0:X8}" -f $debugBase)
        }
    }
}

Write-Host "QSPI loader verification passed: $LoaderPath"
