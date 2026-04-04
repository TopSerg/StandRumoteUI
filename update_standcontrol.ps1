# update_standremoteui.ps1
$ErrorActionPreference = "Stop"

$Repo = "oncominglane/StandRemoteUI"
$Asset = "stand_bundle.zip"
$Url = "https://github.com/$Repo/releases/latest/download/$Asset"

$Base = "C:\StandRemoteUI"
$Dist = Join-Path $Base "dist"
$Tmp  = Join-Path $Base "tmp"
$Zip  = Join-Path $Tmp $Asset

Write-Host "Updating StandRemoteUI..."

New-Item -ItemType Directory -Force -Path $Base, $Dist, $Tmp | Out-Null

Write-Host "Downloading: $Url"
Invoke-WebRequest -Uri $Url -OutFile $Zip -UseBasicParsing

# Stop running processes if they exist
$processes = @("MarathonWS", "app")
foreach ($p in $processes) {
  Get-Process $p -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction SilentlyContinue
}

# Clean dist
Get-ChildItem $Dist -Force -ErrorAction SilentlyContinue |
  Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

# Extract bundle
Expand-Archive -Path $Zip -DestinationPath $Base -Force

# Start applications
Start-Process -WorkingDirectory $Dist -FilePath (Join-Path $Dist "MarathonWS.exe")
Start-Process -WorkingDirectory $Dist -FilePath (Join-Path $Dist "app.exe")

Write-Host "Update complete."

# powershell -ExecutionPolicy Bypass -File C:\work\StandRemoteUI\update_standcontrol.ps1
