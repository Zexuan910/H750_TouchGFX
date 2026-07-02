param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",

    [string]$LoaderPath = "",

    [string]$ProgrammerPath = "",

    [switch]$UseBin,

    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildDir = Join-Path $Root "build\$Config"
$BootHex = Join-Path $BuildDir "H750_TouchGFX_bootloader.hex"
$AppHex = Join-Path $BuildDir "H750_TouchGFX_app.hex"
$AppBin = Join-Path $BuildDir "H750_TouchGFX_app.bin"
$AppAddress = "0x90000000"
$ExpectedLoaderName = "ATK-DNH750_QSPI_W25Q64JV.stldr"
$LoaderSearchRoots = @(
    (Join-Path $Root "tools\loaders"),
    $Root,
    (Join-Path $env:LOCALAPPDATA "stm32cube\bundles\programmer")
)

function Assert-File {
    param(
        [string]$Path,
        [string]$Message
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Message`: $Path"
    }
}

function Find-Programmer {
    if ($ProgrammerPath) {
        Assert-File $ProgrammerPath "STM32_Programmer_CLI.exe was not found"
        return (Resolve-Path -LiteralPath $ProgrammerPath).Path
    }

    $bundleRoot = Join-Path $env:LOCALAPPDATA "stm32cube\bundles\programmer"
    if (Test-Path -LiteralPath $bundleRoot) {
        $candidate = Get-ChildItem -LiteralPath $bundleRoot -Recurse -Filter STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }

    $fromPath = Get-Command STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    throw "STM32_Programmer_CLI.exe was not found. Install STM32CubeProgrammer or pass -ProgrammerPath."
}

function Assert-SupportedLoader {
    param(
        [string]$Path
    )

    $extension = [System.IO.Path]::GetExtension($Path)
    if ($extension -ieq ".flm") {
        throw "Keil/MDK Flash Algorithm files (.FLM) are not supported by STM32CubeProgrammer -el: $Path. Use or build a board-specific .stldr instead."
    }

    if ($extension -ine ".stldr") {
        throw "External loader must be a STM32CubeProgrammer .stldr file: $Path"
    }
}

function Find-Loader {
    if ($LoaderPath) {
        Assert-File $LoaderPath "External loader was not found"
        $resolved = (Resolve-Path -LiteralPath $LoaderPath).Path
        Assert-SupportedLoader $resolved
        return $resolved
    }

    foreach ($searchRoot in $LoaderSearchRoots) {
        if (-not (Test-Path -LiteralPath $searchRoot)) {
            continue
        }

        $exact = Get-ChildItem -LiteralPath $searchRoot -Recurse -Filter $ExpectedLoaderName -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($exact) {
            return $exact.FullName
        }
    }

    foreach ($searchRoot in $LoaderSearchRoots) {
        if (-not (Test-Path -LiteralPath $searchRoot)) {
            continue
        }

        $near = Get-ChildItem -LiteralPath $searchRoot -Recurse -Filter "*.stldr" -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match "DNH750|W25Q64|ATK" } |
            Select-Object -First 1
        if ($near) {
            return $near.FullName
        }
    }

    return $null
}

function Find-UnsupportedFlm {
    foreach ($searchRoot in $LoaderSearchRoots) {
        if (-not (Test-Path -LiteralPath $searchRoot)) {
            continue
        }

        $candidate = Get-ChildItem -LiteralPath $searchRoot -Recurse -Filter "*.FLM" -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match "DNH750|W25Q64|ATK" } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }

    return $null
}

function Format-CommandLine {
    param(
        [string]$Exe,
        [string[]]$CliArgs
    )

    $quoted = @($Exe) + $CliArgs | ForEach-Object {
        if ($_ -match "\s") {
            '"' + $_ + '"'
        } else {
            $_
        }
    }
    return ($quoted -join " ")
}

function Invoke-ProgrammerStep {
    param(
        [string]$Label,
        [string[]]$CliArgs
    )

    Write-Host "[$Label]"
    Write-Host (Format-CommandLine $Programmer $CliArgs)

    if ($DryRun) {
        return
    }

    & $Programmer @CliArgs
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE"
    }
}

Assert-File $BootHex "Missing bootloader HEX. Build the $Config preset first"
Assert-File $AppHex "Missing QSPI app HEX. Build the $Config preset first"
Assert-File $AppBin "Missing QSPI app BIN. Build the $Config preset first"

$Programmer = Find-Programmer
$ResolvedLoader = Find-Loader
$UnsupportedFlm = $null

if (-not $ResolvedLoader) {
    $UnsupportedFlm = Find-UnsupportedFlm
    $message = "Missing external loader $ExpectedLoaderName. Pass -LoaderPath with the board-specific W25Q64JV loader."
    if ($UnsupportedFlm) {
        $message += " Found unsupported Keil/MDK FLM at $UnsupportedFlm; do not rename it to .stldr."
    }
    if ($DryRun) {
        Write-Host $message
    } else {
        throw $message
    }
}

if (-not $ResolvedLoader) {
    Write-Host "[Program app to QSPI NOR Flash first]"
    Write-Host "Blocked until -LoaderPath points to $ExpectedLoaderName."
    exit 0
}

if ($UseBin) {
    $appArgs = @("-c", "port=SWD", "mode=UR", "-el", $ResolvedLoader, "-w", $AppBin, $AppAddress, "-v")
} else {
    $appArgs = @("-c", "port=SWD", "mode=UR", "-el", $ResolvedLoader, "-w", $AppHex, "-v")
}

Invoke-ProgrammerStep "Program app to QSPI NOR Flash first" $appArgs

$bootArgs = @("-c", "port=SWD", "mode=UR", "-w", $BootHex, "-v", "-rst")
Invoke-ProgrammerStep "Program bootloader to internal Flash and reset" $bootArgs
