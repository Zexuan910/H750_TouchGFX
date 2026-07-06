param()

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$IocPath = Join-Path $Root "H750_TouchGFX.ioc"
$MainHeaderPath = Join-Path $Root "Core\Inc\main.h"
$ReadmePath = Join-Path $Root "README_FLASH_DOWNLOAD.txt"
$AgentsPath = Join-Path $Root "AGENTS.MD"

function Assert-File {
    param(
        [string]$Path,
        [string]$Message
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Message`: $Path"
    }
}

function Assert-IocSignal {
    param(
        [string[]]$Lines,
        [string]$Pin,
        [string]$Signal
    )

    $expected = "$Pin.Signal=$Signal"
    if ($Lines -notcontains $expected) {
        throw "Unexpected .ioc pinout for $Pin. Expected '$expected'."
    }
}

function Get-DefineValue {
    param(
        [string[]]$Lines,
        [string]$Name
    )

    $pattern = "^\s*#define\s+$([regex]::Escape($Name))\s+(.+?)\s*$"
    $match = $Lines | Where-Object { $_ -match $pattern } | Select-Object -First 1
    if (-not $match) {
        throw "Missing define $Name in Core\Inc\main.h."
    }

    return ([regex]::Match($match, $pattern).Groups[1].Value)
}

function Assert-MainPin {
    param(
        [string[]]$Lines,
        [string]$Name,
        [string]$ExpectedPort,
        [string]$ExpectedPin
    )

    $actualPin = Get-DefineValue $Lines "${Name}_Pin"
    $actualPort = Get-DefineValue $Lines "${Name}_GPIO_Port"

    if (($actualPin -ne $ExpectedPin) -or ($actualPort -ne $ExpectedPort)) {
        throw "Unexpected main.h pinout for $Name. Expected $ExpectedPort/$ExpectedPin, got $actualPort/$actualPin."
    }
}

function Convert-ToPinName {
    param(
        [string]$PortDefine,
        [string]$PinDefine
    )

    if ($PortDefine -notmatch "^GPIO([A-K])$") {
        throw "Unsupported GPIO port define: $PortDefine"
    }
    $portName = $Matches[1]

    if ($PinDefine -notmatch "^GPIO_PIN_(\d+)$") {
        throw "Unsupported GPIO pin define: $PinDefine"
    }
    $pinName = $Matches[1]

    return ("P{0}{1}" -f $portName, $pinName)
}

function Assert-TextContains {
    param(
        [string]$Path,
        [string]$Expected
    )

    $text = Get-Content -LiteralPath $Path -Raw
    if (-not $text.Contains($Expected)) {
        throw "Missing expected text in $([System.IO.Path]::GetFileName($Path)): $Expected"
    }
}

function Assert-TextNotContains {
    param(
        [string]$Path,
        [string]$Unexpected
    )

    $text = Get-Content -LiteralPath $Path -Raw
    if ($text.Contains($Unexpected)) {
        throw "Unexpected text in $([System.IO.Path]::GetFileName($Path)): $Unexpected"
    }
}

Assert-File $IocPath "Missing CubeMX project"
Assert-File $MainHeaderPath "Missing main.h"
Assert-File $ReadmePath "Missing flash download notes"
Assert-File $AgentsPath "Missing AGENTS.MD"

$iocLines = Get-Content -LiteralPath $IocPath
$mainLines = Get-Content -LiteralPath $MainHeaderPath

$expectedIocSignals = [ordered]@{
    "PB2" = "QUADSPI_CLK"
    "PB6" = "QUADSPI_BK1_NCS"
    "PF8" = "QUADSPI_BK1_IO0"
    "PF9" = "QUADSPI_BK1_IO1"
    "PF7" = "QUADSPI_BK1_IO2"
    "PF6" = "QUADSPI_BK1_IO3"
    "PB8" = "I2C1_SCL"
    "PB9" = "I2C1_SDA"
}

foreach ($entry in $expectedIocSignals.GetEnumerator()) {
    Assert-IocSignal $iocLines $entry.Key $entry.Value
}

$expectedMainPins = @(
    @{ Name = "QSPI_CLK";     Port = "GPIOB"; Pin = "GPIO_PIN_2" },
    @{ Name = "QSPI_NCS";     Port = "GPIOB"; Pin = "GPIO_PIN_6" },
    @{ Name = "QSPI_BK1_IO0"; Port = "GPIOF"; Pin = "GPIO_PIN_8" },
    @{ Name = "QSPI_BK1_IO1"; Port = "GPIOF"; Pin = "GPIO_PIN_9" },
    @{ Name = "QSPI_BK1_IO2"; Port = "GPIOF"; Pin = "GPIO_PIN_7" },
    @{ Name = "QSPI_BK1_IO3"; Port = "GPIOF"; Pin = "GPIO_PIN_6" }
)

foreach ($pin in $expectedMainPins) {
    Assert-MainPin $mainLines $pin.Name $pin.Port $pin.Pin
}

$qspiPhysicalPins = @{}
foreach ($pin in $expectedMainPins) {
    $qspiPhysicalPins[(Convert-ToPinName $pin.Port $pin.Pin)] = $pin.Name
}

$lcdPins = @("LCD_SDA", "LCD_SCK", "LCD_DC", "LCD_RST", "LCD_CS")
$lcdPhysicalPins = @{}
foreach ($name in $lcdPins) {
    $pinDefine = Get-DefineValue $mainLines "${name}_Pin"
    $portDefine = Get-DefineValue $mainLines "${name}_GPIO_Port"
    $physicalPin = Convert-ToPinName $portDefine $pinDefine
    $lcdPhysicalPins[$physicalPin] = $name
    if ($qspiPhysicalPins.ContainsKey($physicalPin)) {
        throw "LCD pin $name conflicts with $($qspiPhysicalPins[$physicalPin]) on $physicalPin."
    }
}

$max30102I2cPins = @("PB8", "PB9")
foreach ($physicalPin in $max30102I2cPins) {
    if ($qspiPhysicalPins.ContainsKey($physicalPin)) {
        throw "MAX30102 I2C1 pin $physicalPin conflicts with $($qspiPhysicalPins[$physicalPin])."
    }
    if ($lcdPhysicalPins.ContainsKey($physicalPin)) {
        throw "MAX30102 I2C1 pin $physicalPin conflicts with LCD pin $($lcdPhysicalPins[$physicalPin])."
    }
}

Assert-TextNotContains $IocPath "PB8.Signal=I2C4_SCL"
Assert-TextNotContains $IocPath "PB9.Signal=I2C4_SDA"
Assert-TextNotContains $IocPath "Mcu.IP11=I2C4"

Assert-TextContains $ReadmePath "QSPI wiring: PB2=CLK, PB6=NCS, PF8=IO0, PF9=IO1, PF7=IO2, PF6=IO3."
Assert-TextContains $AgentsPath '`PF8`：QSPI_BK1_IO0'
Assert-TextContains $AgentsPath '`PF9`：QSPI_BK1_IO1'

Write-Host "Board pinout verification passed."
