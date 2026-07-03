param(
  [ValidateSet("Debug", "Release")]
  [string]$Config = "Debug",

  [string]$LoaderPath = "tools/loaders/ATK-DNH750_QSPI_W25Q64JV.stldr",

  [int]$DebounceSeconds = 3,

  [switch]$InitialRun,

  [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$RunCMake = Join-Path $PSScriptRoot "run_cmake.ps1"
$VerifyLayout = Join-Path $PSScriptRoot "verify_qspi_bootloader_layout.ps1"
$VerifyPinout = Join-Path $PSScriptRoot "verify_board_pinout.ps1"
$VerifyVsync = Join-Path $PSScriptRoot "verify_touchgfx_spi_vsync.ps1"
$Program = Join-Path $PSScriptRoot "program_qspi_split.ps1"

function Get-RelativePath {
  param([string]$Path)

  $rootUri = New-Object System.Uri(($Root.TrimEnd("\", "/") + "\"))
  $pathUri = New-Object System.Uri([System.IO.Path]::GetFullPath($Path))
  return [System.Uri]::UnescapeDataString($rootUri.MakeRelativeUri($pathUri).ToString()).Replace("/", "\")
}

function Test-WatchedPath {
  param([string]$Path)

  if (-not $Path) {
    return $false
  }

  $fullPath = [System.IO.Path]::GetFullPath($Path)
  $relative = $fullPath.Substring($Root.Length).TrimStart("\", "/")
  $fileName = [System.IO.Path]::GetFileName($relative)

  if (@("CMakeLists.txt", "CMakePresets.json", "STM32H750XX_BOOTLOADER.ld", "STM32H750XX_QSPI_APP.ld") -contains $fileName) {
    return $true
  }

  if ($relative -match '^(build|\.git|\.vscode)(\\|/|$)') {
    return $false
  }

  if ($relative -match '^(Core|TouchGFX|cmake|tools)(\\|/)') {
    $extension = [System.IO.Path]::GetExtension($relative).ToLowerInvariant()
    return @(".c", ".cpp", ".h", ".hpp", ".s", ".ld", ".cmake", ".ioc", ".touchgfx", ".ps1") -contains $extension
  }

  return $false
}

function Invoke-Step {
  param(
    [string]$Name,
    [scriptblock]$Action
  )

  Write-Host ""
  Write-Host "== $Name =="
  & $Action
  if ($LASTEXITCODE -ne 0) {
    throw "$Name failed with exit code $LASTEXITCODE"
  }
}

function Invoke-BuildVerifyFlash {
  $start = Get-Date
  Write-Host ""
  Write-Host "[$($start.ToString("HH:mm:ss"))] Change accepted. Build, verify, then flash $Config."

  Push-Location $Root
  try {
    Invoke-Step "Configure $Config" { & $RunCMake --preset $Config }
    Invoke-Step "Build $Config firmware" { & $RunCMake --build --preset $Config }
    Invoke-Step "Verify QSPI bootloader layout" { & powershell -ExecutionPolicy Bypass -File $VerifyLayout -Config $Config }
    Invoke-Step "Verify board pinout" { & powershell -ExecutionPolicy Bypass -File $VerifyPinout }
    Invoke-Step "Verify TouchGFX SPI VSYNC" { & powershell -ExecutionPolicy Bypass -File $VerifyVsync }

    if ($DryRun) {
      Invoke-Step "Dry run flash $Config firmware" {
        & powershell -ExecutionPolicy Bypass -File $Program -Config $Config -LoaderPath $LoaderPath -DryRun
      }
    } else {
      Invoke-Step "Flash $Config firmware" {
        & powershell -ExecutionPolicy Bypass -File $Program -Config $Config -LoaderPath $LoaderPath
      }
    }

    $end = Get-Date
    Write-Host ""
    Write-Host "[$($end.ToString("HH:mm:ss"))] Done."
  } finally {
    Pop-Location
  }
}

$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = $Root
$watcher.IncludeSubdirectories = $true
$watcher.EnableRaisingEvents = $true

$eventNames = @("Changed", "Created", "Deleted", "Renamed")
foreach ($eventName in $eventNames) {
  Register-ObjectEvent -InputObject $watcher -EventName $eventName -SourceIdentifier "WatchFlash.$eventName" | Out-Null
}

Write-Host "Watching $Root"
Write-Host "Config: $Config"
Write-Host "Mode: $(if ($DryRun) { "dry run" } else { "flash" })"
Write-Host "Stop this task with Ctrl+C."

if ($InitialRun) {
  try {
    Invoke-BuildVerifyFlash
  } catch {
    Write-Host ""
    Write-Host "Initial run failed: $($_.Exception.Message)"
  }
}

$pending = $false
$deadline = $null
$lastPath = ""

while ($true) {
  $event = Wait-Event -Timeout 1
  if ($event) {
    $path = $event.SourceEventArgs.FullPath
    Remove-Event -EventIdentifier $event.EventIdentifier

    if (Test-WatchedPath $path) {
      $pending = $true
      $deadline = (Get-Date).AddSeconds($DebounceSeconds)
      $lastPath = $path
      Write-Host "Queued: $(Get-RelativePath $path)"
    }
  }

  while ($event = Get-Event) {
    $path = $event.SourceEventArgs.FullPath
    Remove-Event -EventIdentifier $event.EventIdentifier

    if (Test-WatchedPath $path) {
      $pending = $true
      $deadline = (Get-Date).AddSeconds($DebounceSeconds)
      $lastPath = $path
      Write-Host "Queued: $(Get-RelativePath $path)"
    }
  }

  if ($pending -and $deadline -and (Get-Date) -ge $deadline) {
    $pending = $false
    try {
      Write-Host "Trigger: $(Get-RelativePath $lastPath)"
      Invoke-BuildVerifyFlash
    } catch {
      Write-Host ""
      Write-Host "Auto flash failed: $($_.Exception.Message)"
      Write-Host "Fix the issue, save again, and the watcher will retry."
    }
  }
}
