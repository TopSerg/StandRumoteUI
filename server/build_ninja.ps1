# build_ninja.ps1 — clean configure build (Ninja + CMake)
# Работает в PowerShell 5+. Без кириллицы, чтобы не было проблем с кодировкой.

param(
  [string]$VcpkgToolchain = "C:\vcpkg\scripts\buildsystems\vcpkg.cmake",
  [string]$Triplet = "x64-windows",
  [int]$Jobs = 12,
  [switch]$NoVcpkg   # если указан, не использовать vcpkg toolchain
)

$ErrorActionPreference = 'Stop'

function Test-Cmd([string]$cmd, [string]$args="--version") {
  try { & $cmd $args | Out-Null; return $true } catch { return $false }
}

# 1) Ninja: если нет в PATH — скачать локально и распаковать корректно
if (-not (Test-Cmd "ninja")) {
  Write-Host "Ninja not found. Downloading local copy..."
  $tools = Join-Path $PSScriptRoot ".tools"
  New-Item -ItemType Directory -Force -Path $tools | Out-Null
  $zip = Join-Path $tools "ninja-win.zip"
  $ninjaExe = Join-Path $tools "ninja.exe"
  $url = "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip"

  if (-not (Test-Path $ninjaExe)) {
    # Скачиваем zip
    if (Test-Cmd "curl") {
      curl -L $url -o $zip | Out-Null
    } else {
      $wc = New-Object System.Net.WebClient
      $wc.DownloadFile($url, $zip)
    }
    # Распаковка без третьего аргумента (PS5 совместимо)
    if (Get-Command Expand-Archive -ErrorAction SilentlyContinue) {
      Expand-Archive -Path $zip -DestinationPath $tools -Force
    } else {
      Add-Type -AssemblyName System.IO.Compression.FileSystem
      [IO.Compression.ZipFile]::ExtractToDirectory($zip, $tools)
    }
    Remove-Item $zip -Force
  }
  $env:PATH = "$tools;$env:PATH"
  Write-Host "Using Ninja at $ninjaExe"
}

# 2) Проверка компилятора MSVC (нужен 'cl').
# Проще всего запустить этот скрипт из терминала
# 'x64 Native Tools Command Prompt for VS 2022'.
if (-not (Test-Cmd "cl" "")) {
  Write-Host "MSVC compiler 'cl' not found." -ForegroundColor Red
  Write-Host "Open 'x64 Native Tools Command Prompt for VS 2022' and run:" -ForegroundColor Yellow
  Write-Host "  powershell -ExecutionPolicy Bypass -File .\build_ninja.ps1" -ForegroundColor Yellow
  exit 1
}

# 3) Чистим build
$buildDir = Join-Path $PSScriptRoot "build"
Write-Host "Cleaning build dir: $buildDir"
Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue

# 4) Конфигурация CMake
Write-Host "Configuring with CMake (Ninja)..."
$cfg = @("-S", $PSScriptRoot, "-B", $buildDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release")

if (-not $NoVcpkg) {
  if (Test-Path $VcpkgToolchain) {
    $cfg += "-DCMAKE_TOOLCHAIN_FILE=$VcpkgToolchain"
    $cfg += "-DVCPKG_TARGET_TRIPLET=$Triplet"
  } else {
    Write-Host "vcpkg toolchain not found at $VcpkgToolchain. Proceeding without vcpkg..." -ForegroundColor Yellow
  }
}

# Временная совместимость с старым FindBoost, если нужно
$cfg += @("-D", "CMAKE_POLICY_DEFAULT_CMP0167=OLD")

& cmake @cfg
if ($LASTEXITCODE -ne 0) { Write-Host "Configuration failed"; exit 1 }

# 5) Сборка
Write-Host "Building..."
& cmake --build $buildDir --parallel $Jobs
if ($LASTEXITCODE -ne 0) { Write-Host "Build failed"; exit 1 }

# 6) Результаты
Write-Host "Build artifacts (.exe):"
Get-ChildItem -Recurse -Filter *.exe $buildDir | Select-Object FullName