param(
  [string]$Root = "."
)

$ErrorActionPreference = "Stop"

$uiProcessPath = Join-Path $Root "Core/Src/ui_touchgfx.c"
$bridgePath = Join-Path $Root "Core/Src/ui_touchgfx_vsync.cpp"
$cmakePath = Join-Path $Root "CMakeLists.txt"

foreach ($path in @($uiProcessPath, $bridgePath, $cmakePath)) {
  if (-not (Test-Path $path)) {
    throw "Required file is missing: $path"
  }
}

$uiProcess = Get-Content $uiProcessPath -Raw
$bridge = Get-Content $bridgePath -Raw
$cmake = Get-Content $cmakePath -Raw

$uiProcessFn = [regex]::Match(
  $uiProcess,
  'void\s+UI_Process\s*\(\s*void\s*\)\s*\{(?s).*?\n\}'
).Value

if (-not $uiProcessFn) {
  throw "UI_Process(void) was not found in $uiProcessPath"
}

$signalIndex = $uiProcessFn.IndexOf("UI_TouchGFX_SignalVSync")
$processIndex = $uiProcessFn.IndexOf("MX_TouchGFX_Process")

if ($signalIndex -lt 0) {
  throw "UI_Process() must signal a software VSYNC before processing TouchGFX"
}
if ($processIndex -lt 0) {
  throw "UI_Process() must call MX_TouchGFX_Process()"
}
if ($signalIndex -gt $processIndex) {
  throw "UI_TouchGFX_SignalVSync() must run before MX_TouchGFX_Process()"
}

if ($bridge -notmatch 'OSWrappers::signalVSync\s*\(') {
  throw "$bridgePath must call touchgfx::OSWrappers::signalVSync()"
}

if ($cmake -notmatch 'Core/Src/ui_touchgfx_vsync\.cpp') {
  throw "CMakeLists.txt must compile Core/Src/ui_touchgfx_vsync.cpp for the TouchGFX backend"
}

Write-Host "TouchGFX SPI software VSYNC verification passed."
