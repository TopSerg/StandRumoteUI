# make_bundle.ps1
# Run this script from the repository root (e.g. C:\work\StandControl):
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\make_bundle.ps1
#
# Creates stand_bundle.zip containing:
#   - server/build/MarathonWS.exe
#   - client/dist/app.exe
#   - required vendor DLLs from server/Stand_Marathon/lib (ALL *.dll)
#   - PCANBasic.dll (from build OR vendor lib OR System32 as fallback)
#   - MSVC runtime DLLs (msvcp140.dll, vcruntime140.dll, vcruntime140_1.dll) from VS Redist
#
# Output (relative to repo root):
#   .\bundle_out\stand_bundle.zip
#   .\bundle_out\manifest.txt
#
# Notes:
# - Does NOT copy arbitrary System32 DLLs (except PCANBasic.dll as a special-case fallback).
# - Ignores api-ms-win-*/ext-ms-* entries (api-set, not real files to ship).

$ErrorActionPreference = "Stop"

# === REPO ROOT (must be current directory) ===
$RepoRoot = (Get-Location).Path

# === CONFIG (relative paths) ===
$Dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"

$ServerExeRel = "server\build\MarathonWS.exe"
$ClientExeRel = "client\dist\app.exe"

# Vendor DLLs that MUST be included (all *.dll from this folder)
$VendorLibDirRel = "server\Stand_Marathon\lib"

$VcpkgRoot = "C:\vcpkg"   # keep absolute (toolchain install)

$OutDirRel   = "bundle_out"
$StageDirRel = Join-Path $OutDirRel "stage"
$DistDirRel  = Join-Path $StageDirRel "dist"
$ZipOutRel   = Join-Path $OutDirRel "stand_bundle.zip"
$ManifestRel = Join-Path $OutDirRel "manifest.txt"

# === RESOLVED ABSOLUTE PATHS ===
$ServerExe   = Join-Path $RepoRoot $ServerExeRel
$ClientExe   = Join-Path $RepoRoot $ClientExeRel
$VendorLibDir = Join-Path $RepoRoot $VendorLibDirRel

$OutDir   = Join-Path $RepoRoot $OutDirRel
$StageDir = Join-Path $RepoRoot $StageDirRel
$DistDir  = Join-Path $RepoRoot $DistDirRel
$ZipOut   = Join-Path $RepoRoot $ZipOutRel
$Manifest = Join-Path $RepoRoot $ManifestRel

# === HELPERS ===
function Assert-PathExists([string]$Path, [string]$Label) {
  if (-not (Test-Path $Path)) { throw "$Label not found: $Path" }
}

function Get-DumpbinDependents([string]$ExePath) {
  $out = & $Dumpbin /nologo /dependents $ExePath 2>$null
  if (-not $out) { return @() }

  $dlls = @()
  foreach ($line in $out) {
    $m = [regex]::Match($line, "^\s*([A-Za-z0-9._-]+\.dll)\s*$", "IgnoreCase")
    if ($m.Success) { $dlls += $m.Groups[1].Value }
  }
  return $dlls | Sort-Object -Unique
}

function Test-SystemDllName([string]$Dll) {
  $n = $Dll.ToLowerInvariant()
  return @(
    "kernel32.dll","ws2_32.dll","user32.dll","gdi32.dll","advapi32.dll","comctl32.dll"
  ) -contains $n
}

function Test-ApiSetName([string]$Dll) {
  $n = $Dll.ToLowerInvariant()
  return ($n.StartsWith("api-ms-win-") -or $n.StartsWith("ext-ms-"))
}

function Find-FileInDirs([string]$File, [string[]]$Dirs) {
  foreach ($d in $Dirs) {
    if (-not $d) { continue }
    $cand = Join-Path $d $File
    if (Test-Path $cand) { return (Resolve-Path $cand).Path }
  }
  return $null
}

function Find-FirstInPath([string]$Name) {
  try {
    $res = & where.exe $Name 2>$null
    if ($res) {
      foreach ($r in $res) {
        if ($r -and (Test-Path $r)) { return (Resolve-Path $r).Path }
      }
    }
  } catch {}
  return $null
}

function Find-MsvcCrtRedistDir() {
  $roots = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC"
  )

  foreach ($root in $roots) {
    if (-not (Test-Path $root)) { continue }

    $verDirs = Get-ChildItem $root -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
    foreach ($v in $verDirs) {
      $cand = Join-Path $v.FullName "x64\Microsoft.VC143.CRT"
      if (Test-Path $cand) { return $cand }

      $cand2 = Join-Path $v.FullName "onecore\x64\Microsoft.VC143.CRT"
      if (Test-Path $cand2) { return $cand2 }
    }
  }
  return $null
}

function Copy-FileToDist([string]$SrcPath, [string]$Note, $CopiedList) {
  $dst = Join-Path $DistDir (Split-Path $SrcPath -Leaf)
  Copy-Item $SrcPath $dst -Force
  $CopiedList.Add("$Note <- $SrcPath") | Out-Null
}

# === START ===
Write-Host "make_bundle: start"
Write-Host "RepoRoot: $RepoRoot"

Assert-PathExists $Dumpbin  "dumpbin.exe"
Assert-PathExists $ServerExe "Server EXE"
Assert-PathExists $ClientExe "Client EXE"
Assert-PathExists $VendorLibDir "Vendor lib dir"

New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
if (Test-Path $StageDir) { Remove-Item $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

$system32 = "$env:WINDIR\System32"
$copied  = New-Object System.Collections.Generic.List[string]
$missing = New-Object System.Collections.Generic.List[string]
$forced  = New-Object System.Collections.Generic.List[string]

# Copy EXEs
Copy-Item $ServerExe (Join-Path $DistDir "MarathonWS.exe") -Force
Copy-Item $ClientExe (Join-Path $DistDir "app.exe") -Force

# === FORCE INCLUDE: vendor DLLs from server/Stand_Marathon/lib ===
$vendorDlls = Get-ChildItem $VendorLibDir -Filter "*.dll" -File -ErrorAction SilentlyContinue
if (-not $vendorDlls -or $vendorDlls.Count -eq 0) {
  throw "Vendor lib dir has no DLLs: $VendorLibDir"
}

foreach ($f in $vendorDlls) {
  Copy-FileToDist $f.FullName ("VENDOR " + $f.Name) $copied
  $forced.Add($f.Name.ToLowerInvariant()) | Out-Null
}

# Dependency lists (top-level only)
$serverDeps = Get-DumpbinDependents $ServerExe
$clientDeps = Get-DumpbinDependents $ClientExe

Write-Host "Server deps: $($serverDeps -join ', ')"
Write-Host "Client deps: $($clientDeps -join ', ')"

$need = @()
$need += $serverDeps
$need += $clientDeps
$need = $need | Sort-Object -Unique

# Search paths for non-system DLLs (no System32 here!)
$searchDirs = @(
  (Split-Path $ServerExe -Parent),
  (Split-Path $ClientExe -Parent),
  $VendorLibDir,
  (Join-Path $VcpkgRoot "installed\x64-windows\bin"),
  (Join-Path $VcpkgRoot "installed\x64-windows\debug\bin")
)

# Copy required non-system DLLs (if not already forced)
foreach ($dll in $need) {
  if (Test-SystemDllName $dll) { continue }
  if (Test-ApiSetName $dll) { continue }

  $dllLower = $dll.ToLowerInvariant()

  # already copied from vendor folder (or earlier)
  if ($forced -contains $dllLower) { continue }
  if (Test-Path (Join-Path $DistDir $dll)) { continue }

  $path = Find-FileInDirs $dll $searchDirs
  if (-not $path) { $path = Find-FirstInPath $dll }

  # Special-case: PCANBasic.dll may be in build/vendor/system32
  if (-not $path -and $dllLower -eq "pcanbasic.dll") {
    $path = Find-FileInDirs $dll @((Split-Path $ServerExe -Parent), $VendorLibDir, $system32)
  }

  if (-not $path) {
    $missing.Add($dll) | Out-Null
    continue
  }

  # Do NOT copy arbitrary System32 DLLs
  if ($path.ToLowerInvariant().StartsWith($system32.ToLowerInvariant()) -and $dllLower -ne "pcanbasic.dll") {
    continue
  }

  Copy-FileToDist $path $dll $copied
}

# Force-copy MSVC runtime DLLs from MSVC CRT redist folder (preferred)
$crtDir = Find-MsvcCrtRedistDir
if ($crtDir) {
  foreach ($r in @("msvcp140.dll","vcruntime140.dll","vcruntime140_1.dll")) {
    $p = Join-Path $crtDir $r
    if (Test-Path $p) {
      Copy-FileToDist $p $r $copied
    } else {
      Write-Warning "Runtime DLL not found in CRT dir: $p"
    }
  }
} else {
  Write-Warning "CRT redist folder not found; runtime DLLs not bundled"
}

# Manifest
$lines = @()
$lines += "Created: $(Get-Date -Format s)"
$lines += "RepoRoot: $RepoRoot"
$lines += "Dumpbin: $Dumpbin"
$lines += "ServerExe: $ServerExeRel"
$lines += "ClientExe: $ClientExeRel"
$lines += "VendorLibDir: $VendorLibDirRel"
$lines += ""
$lines += "Files in dist:"
Get-ChildItem $DistDir | Sort-Object Name | ForEach-Object { $lines += "  - " + $_.Name }
$lines += ""
$lines += "Copied files:"
if ($copied.Count -gt 0) { $copied | ForEach-Object { $lines += "  " + $_ } } else { $lines += "  (none)" }
$lines += ""
if ($missing.Count -gt 0) {
  $lines += "MISSING DLLs:"
  ($missing | Sort-Object -Unique) | ForEach-Object { $lines += "  - " + $_ }
} else {
  $lines += "Missing DLLs: none"
}

$lines | Set-Content -Encoding ASCII $Manifest

# Zip
if (Test-Path $ZipOut) { Remove-Item $ZipOut -Force }
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $ZipOut -Force

Write-Host "make_bundle: OK"
Write-Host "ZIP: $ZipOutRel"
Write-Host "Manifest: $ManifestRel"

if ($missing.Count -gt 0) {
  Write-Warning "Missing DLLs exist. Check manifest.txt"
}

# Run:
#   cd C:\work\StandControl
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\make_bundle.ps1
