param(
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildDir = Join-Path $Root "build\$Config"
$InternalFlashStart = [Convert]::ToUInt64("08000000", 16)
$InternalFlashEnd = [Convert]::ToUInt64("08020000", 16)
$QspiStart = [Convert]::ToUInt64("90000000", 16)
$QspiEnd = [Convert]::ToUInt64("90800000", 16)
$AxiRamStart = [Convert]::ToUInt64("24000000", 16)
$AxiRamEnd = [Convert]::ToUInt64("24080000", 16)

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Read-RequiredText {
    param([string]$Path)

    Assert-True (Test-Path -LiteralPath $Path) "Missing required file: $Path"
    return Get-Content -Raw -LiteralPath $Path
}

function Find-ArmSize {
    $localTools = Join-Path $env:LOCALAPPDATA "stm32cube\bundles\gnu-tools-for-stm32"
    if (Test-Path -LiteralPath $localTools) {
        $candidate = Get-ChildItem -LiteralPath $localTools -Recurse -Filter arm-none-eabi-size.exe -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }

    $fromPath = Get-Command arm-none-eabi-size.exe -ErrorAction SilentlyContinue
    Assert-True ($null -ne $fromPath) "arm-none-eabi-size.exe was not found"
    return $fromPath.Source
}

function Get-Sections {
    param([string]$ElfPath)

    Assert-True (Test-Path -LiteralPath $ElfPath) "Missing ELF: $ElfPath"
    $sizeTool = Find-ArmSize
    $output = & $sizeTool -A $ElfPath
    Assert-True ($LASTEXITCODE -eq 0) "arm-none-eabi-size failed for $ElfPath"

    $sections = @{}
    foreach ($line in $output) {
        if ($line -match '^\s*(\S+)\s+(\d+)\s+(\d+)\s*$') {
            $sections[$matches[1]] = @{
                Size = [uint64]$matches[2]
                Addr = [uint64]$matches[3]
            }
        }
    }

    return $sections
}

function Assert-SectionInRange {
    param(
        [hashtable]$Sections,
        [string]$Name,
        [uint64]$Start,
        [uint64]$End
    )

    Assert-True ($Sections.ContainsKey($Name)) "Missing section $Name"
    $addr = $Sections[$Name].Addr
    Assert-True (($addr -ge $Start) -and ($addr -lt $End)) ("Section {0} at 0x{1:X8} is outside 0x{2:X8}..0x{3:X8}" -f $Name, $addr, $Start, $End)
}

$cmake = Read-RequiredText (Join-Path $Root "CMakeLists.txt")
$appMain = Read-RequiredText (Join-Path $Root "Core\Src\main.c")
$bootLd = Read-RequiredText (Join-Path $Root "STM32H750XX_BOOTLOADER.ld")
$appLd = Read-RequiredText (Join-Path $Root "STM32H750XX_QSPI_APP.ld")
$bootMain = Read-RequiredText (Join-Path $Root "Core\Src\bootloader_main.c")
$launchJson = Read-RequiredText (Join-Path $Root ".vscode\launch.json")

Assert-True ($cmake -match 'H750_TouchGFX_bootloader') "CMake does not define H750_TouchGFX_bootloader"
Assert-True ($cmake -match 'H750_TouchGFX_app') "CMake does not define H750_TouchGFX_app"
Assert-True ($cmake -match 'APP_EXECUTES_FROM_QSPI=1') "App target does not define APP_EXECUTES_FROM_QSPI=1"
Assert-True ($cmake -match 'BOOTLOADER_BUILD=1') "Bootloader target does not define BOOTLOADER_BUILD=1"

Assert-True ($bootLd -match 'ORIGIN\s*=\s*0x08000000') "Bootloader linker script does not start at internal Flash"
Assert-True ($appLd -match 'ORIGIN\s*=\s*0x90000000') "QSPI app linker script does not start at external QSPI"
Assert-True ($appLd -match '\.isr_vector') "QSPI app linker script does not place the vector table"
Assert-True ($appLd -match 'TouchGFX_ExtFlash') "QSPI app linker script does not keep TouchGFX external resources"

Assert-True ($bootMain -match 'SCB->VTOR\s*=\s*APP_ADDR') "Bootloader does not relocate VTOR to the QSPI app"
Assert-True ($bootMain -match '__set_MSP') "Bootloader does not set MSP before jumping"
Assert-True ($bootMain -match 'norflash_memory_mapped') "Bootloader does not enable QSPI memory-mapped mode"

Assert-True ($appMain -match 'APP_EXECUTES_FROM_QSPI') "App main does not guard external flash init for QSPI execution"
Assert-True ($appMain -match 'MPU_INSTRUCTION_ACCESS_ENABLE') "QSPI MPU region is not executable"

Assert-True ($launchJson -notmatch 'get-projects-binary-from-context') "VS Code debug still auto-selects an ELF and can pick stale H750_TouchGFX.elf"
Assert-True ($launchJson -match 'H750_TouchGFX_bootloader\.elf') "VS Code debug does not point at H750_TouchGFX_bootloader.elf"
Assert-True ($launchJson -notmatch 'H750_TouchGFX_app\.elf') "VS Code debug must not download the QSPI app ELF"

$bootElf = Join-Path $BuildDir "H750_TouchGFX_bootloader.elf"
$appElf = Join-Path $BuildDir "H750_TouchGFX_app.elf"
$bootHex = Join-Path $BuildDir "H750_TouchGFX_bootloader.hex"
$appHex = Join-Path $BuildDir "H750_TouchGFX_app.hex"
$appBin = Join-Path $BuildDir "H750_TouchGFX_app.bin"

Assert-True (Test-Path -LiteralPath $bootHex) "Missing bootloader HEX: $bootHex"
Assert-True (Test-Path -LiteralPath $appHex) "Missing app HEX: $appHex"
Assert-True (Test-Path -LiteralPath $appBin) "Missing app BIN: $appBin"

$bootSections = Get-Sections $bootElf
$appSections = Get-Sections $appElf

Assert-SectionInRange $bootSections ".isr_vector" $InternalFlashStart $InternalFlashEnd
Assert-SectionInRange $bootSections ".text" $InternalFlashStart $InternalFlashEnd
Assert-SectionInRange $bootSections ".rodata" $InternalFlashStart $InternalFlashEnd

$bootFlashBytes = 0
foreach ($sectionName in @(".isr_vector", ".text", ".rodata", ".ARM", ".preinit_array", ".init_array", ".fini_array", ".data")) {
    if ($bootSections.ContainsKey($sectionName)) {
        $bootFlashBytes += $bootSections[$sectionName].Size
    }
}
Assert-True ($bootFlashBytes -lt 131072) "Bootloader loadable Flash usage is $bootFlashBytes bytes, expected < 128KB"

Assert-SectionInRange $appSections ".isr_vector" $QspiStart $QspiEnd
Assert-SectionInRange $appSections ".text" $QspiStart $QspiEnd
Assert-SectionInRange $appSections ".rodata" $QspiStart $QspiEnd
Assert-SectionInRange $appSections "TouchGFX_ExtFlash" $QspiStart $QspiEnd
Assert-SectionInRange $appSections "TouchGFX_Framebuffer" $AxiRamStart $AxiRamEnd

$bootHexText = Get-Content -Raw -LiteralPath $bootHex
$appHexText = Get-Content -Raw -LiteralPath $appHex
Assert-True ($bootHexText.Contains(":020000040800F2")) "Bootloader HEX does not contain internal Flash extended address record"
Assert-True ($appHexText.Contains(":0200000490006A")) "App HEX does not contain QSPI extended address record"

Write-Host "QSPI bootloader layout verification passed for $Config"
