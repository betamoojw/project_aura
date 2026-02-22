param(
  [string]$Env = "project_aura",
  [string]$Version,
  [string]$Repo = "21cncstudio/project_aura",
  [string]$Tag,
  [string]$OutputRoot = "release-assets",
  [switch]$SkipBuild,
  [switch]$SkipWebInstallerSync
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
  param([string]$Message)
  Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

function Resolve-PlatformioCommand {
  $preferred = Join-Path $env:USERPROFILE ".platformio\\penv\\Scripts\\platformio.exe"
  if (Test-Path $preferred) {
    return $preferred
  }

  $fallback = Get-Command platformio -ErrorAction SilentlyContinue
  if ($fallback) {
    return $fallback.Path
  }

  throw "PlatformIO executable not found. Expected: $preferred"
}

function Invoke-Platformio {
  param(
    [string]$Exe,
    [string[]]$PioArgs
  )

  & $Exe @PioArgs
  if ($LASTEXITCODE -ne 0) {
    throw "PlatformIO failed: $Exe $($PioArgs -join ' ')"
  }
}

function Get-PartitionOffset {
  param(
    [string]$CsvPath,
    [string]$Name
  )

  if (-not (Test-Path $CsvPath)) {
    return $null
  }

  $lines = Get-Content $CsvPath | Where-Object { $_ -and $_ -notmatch "^\s*#" }
  foreach ($line in $lines) {
    $parts = $line.Split(",") | ForEach-Object { $_.Trim() }
    if ($parts.Count -ge 4 -and $parts[0] -eq $Name) {
      return $parts[3]
    }
  }

  return $null
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$platformioIni = Join-Path $root "platformio.ini"
$buildDir = Join-Path $root (".pio\\build\\{0}" -f $Env)

if (-not $Version -and (Test-Path $platformioIni)) {
  $iniText = Get-Content $platformioIni -Raw
  $match = [regex]::Match($iniText, '-DAPP_VERSION=\\?"?([0-9A-Za-z._-]+)\\?"?')
  if ($match.Success) {
    $Version = $match.Groups[1].Value
  }
}

if (-not $Version) {
  throw "Version not found. Pass -Version 1.1.0 or set -DAPP_VERSION in platformio.ini."
}

if (-not $Tag) {
  $Tag = "v$Version"
}

$platformioExe = Resolve-PlatformioCommand

if (-not $SkipBuild) {
  Write-Step "Building firmware"
  Invoke-Platformio -Exe $platformioExe -PioArgs @("run", "-e", $Env)
  Write-Step "Building filesystem image"
  Invoke-Platformio -Exe $platformioExe -PioArgs @("run", "-e", $Env, "-t", "buildfs")
}

$required = @("bootloader.bin", "partitions.bin", "firmware.bin", "littlefs.bin")
foreach ($name in $required) {
  $path = Join-Path $buildDir $name
  if (-not (Test-Path $path)) {
    throw "Missing build output: $path"
  }
}

$bootApp0 = Join-Path $env:USERPROFILE ".platformio\\packages\\framework-arduinoespressif32\\tools\\partitions\\boot_app0.bin"
if (-not (Test-Path $bootApp0)) {
  throw "Missing boot_app0.bin at $bootApp0"
}

$partitionsCsv = $null
if (Test-Path $platformioIni) {
  $iniText = Get-Content $platformioIni -Raw
  $match = [regex]::Match($iniText, "board_build\\.partitions\\s*=\\s*(\\S+)")
  if ($match.Success) {
    $partitionsCsv = Join-Path $root $match.Groups[1].Value
  }
}
if (-not $partitionsCsv -or -not (Test-Path $partitionsCsv)) {
  $partitionsCsv = Join-Path $root "partitions_16MB_littlefs.csv"
}

$app0Offset = Get-PartitionOffset -CsvPath $partitionsCsv -Name "app0"
if (-not $app0Offset) {
  $app0Offset = "0x10000"
}

$littlefsOffset = Get-PartitionOffset -CsvPath $partitionsCsv -Name "littlefs"
if (-not $littlefsOffset) {
  $littlefsOffset = Get-PartitionOffset -CsvPath $partitionsCsv -Name "spiffs"
}
if (-not $littlefsOffset) {
  $littlefsOffset = "0xC90000"
}

$outDir = Join-Path $root (Join-Path $OutputRoot $Tag)
New-Item -ItemType Directory -Force $outDir | Out-Null

Write-Step "Copying release binaries"
Copy-Item -Force (Join-Path $buildDir "bootloader.bin") (Join-Path $outDir "bootloader.bin")
Copy-Item -Force (Join-Path $buildDir "partitions.bin") (Join-Path $outDir "partitions.bin")
Copy-Item -Force (Join-Path $buildDir "firmware.bin") (Join-Path $outDir "firmware.bin")
Copy-Item -Force (Join-Path $buildDir "littlefs.bin") (Join-Path $outDir "littlefs.bin")
Copy-Item -Force $bootApp0 (Join-Path $outDir "boot_app0.bin")

$otaFileName = "project_aura_{0}_ota_firmware.bin" -f $Version
Copy-Item -Force (Join-Path $buildDir "firmware.bin") (Join-Path $outDir $otaFileName)

$baseUrl = "https://github.com/$Repo/releases/download/$Tag"
$manifestFull = [ordered]@{
  name = "Project Aura"
  version = $Version
  builds = @(
    [ordered]@{
      chipFamily = "ESP32-S3"
      parts = @(
        [ordered]@{ path = "$baseUrl/bootloader.bin"; offset = "0x0000" }
        [ordered]@{ path = "$baseUrl/partitions.bin"; offset = "0x8000" }
        [ordered]@{ path = "$baseUrl/boot_app0.bin"; offset = "0xE000" }
        [ordered]@{ path = "$baseUrl/firmware.bin"; offset = $app0Offset }
        [ordered]@{ path = "$baseUrl/littlefs.bin"; offset = $littlefsOffset }
      )
    }
  )
}
$manifestUpdate = [ordered]@{
  name = "Project Aura"
  version = $Version
  new_install_prompt_erase = $true
  builds = @(
    [ordered]@{
      chipFamily = "ESP32-S3"
      parts = @(
        [ordered]@{ path = "$baseUrl/firmware.bin"; offset = $app0Offset }
      )
    }
  )
}

Write-Step "Writing release manifests"
$manifestFull | ConvertTo-Json -Depth 8 | Set-Content -Encoding Ascii (Join-Path $outDir "manifest.json")
$manifestUpdate | ConvertTo-Json -Depth 8 | Set-Content -Encoding Ascii (Join-Path $outDir "manifest-update.json")

$releaseNotes = Join-Path $root ("docs\\releases\\v{0}.md" -f $Version)
if (Test-Path $releaseNotes) {
  Copy-Item -Force $releaseNotes (Join-Path $outDir ("release-notes-v{0}.md" -f $Version))
}

$hashFiles = @(
  "bootloader.bin",
  "partitions.bin",
  "boot_app0.bin",
  "firmware.bin",
  "littlefs.bin",
  "manifest.json",
  "manifest-update.json",
  $otaFileName
)
$hashLines = foreach ($fileName in $hashFiles) {
  $filePath = Join-Path $outDir $fileName
  $hash = (Get-FileHash -Algorithm SHA256 -Path $filePath).Hash.ToLowerInvariant()
  "$hash  $fileName"
}
$hashLines | Set-Content -Encoding Ascii (Join-Path $outDir "sha256sums.txt")

if (-not $SkipWebInstallerSync) {
  $webPrepareScript = Join-Path $root "web-installer\\prepare_web_installer.ps1"
  if (Test-Path $webPrepareScript) {
    Write-Step "Syncing local web-installer files"
    & powershell -ExecutionPolicy Bypass -File $webPrepareScript -Env $Env -Version $Version -SkipBuild
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to sync web-installer files."
    }
  } else {
    Write-Warning "web-installer\\prepare_web_installer.ps1 not found, skipping local web-installer sync."
  }
}

Write-Host ""
Write-Host "Release assets prepared in: $outDir"
Write-Host "Tag: $Tag"
Write-Host "Upload this file to GitHub Release:"
Write-Host " - $otaFileName"
Write-Host "Other files in release-assets are for local installer workflow."
Write-Host ""
Write-Host "Local website installer files are refreshed in: $root\\web-installer"
