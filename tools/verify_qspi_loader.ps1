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
$expectedPageSize = [Convert]::ToUInt32("00001000", 16)
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
