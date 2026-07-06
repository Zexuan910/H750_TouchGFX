$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Assert-FileExists {
    param([string]$Path)
    $fullPath = Join-Path $repoRoot $Path
    if (-not (Test-Path -LiteralPath $fullPath)) {
        throw "Missing expected file: $Path"
    }
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$Pattern,
        [string]$Description
    )

    $fullPath = Join-Path $repoRoot $Path
    $content = Get-Content -LiteralPath $fullPath -Raw
    if ($content -notmatch $Pattern) {
        throw "Missing $Description in $Path"
    }
}

$touchgfxFile = "TouchGFX/H750_TouchGFX.touchgfx"
$heapFile = "TouchGFX/generated/gui_generated/include/gui_generated/common/FrontendHeapBase.hpp"
$appFile = "TouchGFX/gui/include/gui/common/FrontendApplication.hpp"
$appImplFile = "TouchGFX/gui/src/common/FrontendApplication.cpp"
$cmakeFile = "cmake/touchgfx/CMakeLists.txt"

Assert-FileExists $touchgfxFile
Assert-FileExists $heapFile
Assert-FileExists $appFile
Assert-FileExists $appImplFile
Assert-FileExists $cmakeFile

foreach ($screen in @("home", "lock", "screen1", "walk", "run", "rope")) {
    Assert-Contains $touchgfxFile """Name""\s*:\s*""$screen""" "TouchGFX screen '$screen'"
    Assert-Contains $heapFile "$($screen)View" "heap view type for '$screen'"
    Assert-Contains $heapFile "$($screen)Presenter" "heap presenter type for '$screen'"
}

foreach ($screen in @("walk", "run", "rope")) {
    Assert-FileExists "TouchGFX/gui/include/gui/$($screen)_screen/$($screen)View.hpp"
    Assert-FileExists "TouchGFX/gui/src/$($screen)_screen/$($screen)View.cpp"
    Assert-FileExists "TouchGFX/gui/include/gui/$($screen)_screen/$($screen)Presenter.hpp"
    Assert-FileExists "TouchGFX/gui/src/$($screen)_screen/$($screen)Presenter.cpp"
    Assert-FileExists "TouchGFX/generated/gui_generated/include/gui_generated/$($screen)_screen/$($screen)ViewBase.hpp"
    Assert-FileExists "TouchGFX/generated/gui_generated/src/$($screen)_screen/$($screen)ViewBase.cpp"

    Assert-Contains $appFile "goto$($screen)ScreenNoTransition" "FrontendApplication transition declaration for '$screen'"
    Assert-Contains $appImplFile "makeTransition<$($screen)View,\s*$($screen)Presenter" "FrontendApplication transition implementation for '$screen'"
    Assert-Contains $cmakeFile "$($screen)_screen/$($screen)View\.cpp" "CMake view source for '$screen'"
    Assert-Contains $cmakeFile "$($screen)_screen/$($screen)Presenter\.cpp" "CMake presenter source for '$screen'"
    Assert-Contains "TouchGFX/gui/src/$($screen)_screen/$($screen)View.cpp" "SENSOR ERR" "$screen sensor missing state"
    Assert-Contains "TouchGFX/gui/src/$($screen)_screen/$($screen)View.cpp" "PLACE FINGER" "$screen no-finger state"
    Assert-Contains "TouchGFX/gui/src/$($screen)_screen/$($screen)View.cpp" "MEASURING" "$screen measuring state"
    Assert-Contains "TouchGFX/gui/src/$($screen)_screen/$($screen)Presenter.cpp" "watchDataUpdated" "$screen presenter watch data forwarding"
}

Assert-Contains "TouchGFX/gui/src/home_screen/homeView.cpp" "gotoscreen1ScreenNoTransition" "home to navigation transition"
Assert-Contains "TouchGFX/gui/src/screen1_screen/screen1View.cpp" "gotowalkScreenNoTransition" "navigation to walk transition"
Assert-Contains "TouchGFX/gui/src/screen1_screen/screen1View.cpp" "gotorunScreenNoTransition" "navigation to run transition"
Assert-Contains "TouchGFX/gui/src/screen1_screen/screen1View.cpp" "gotoropeScreenNoTransition" "navigation to rope transition"

Write-Host "TouchGFX six-screen guard passed."
