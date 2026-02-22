param(
  [Parameter(Mandatory = $true)]
  [string]$Version,
  [string]$Repo = "21cncstudio/project_aura",
  [string]$Tag,
  [string]$Title,
  [string]$AssetsDir,
  [string]$NotesPath,
  [string]$TargetCommitish = "main",
  [int]$ApiTimeoutSec = 60,
  [int]$UploadTimeoutSec = 300,
  [int]$ConnectTimeoutSec = 15,
  [switch]$Draft,
  [switch]$SkipNotes,
  [switch]$SkipReleaseUpdate,
  [switch]$PruneAssetsToList,
  [string[]]$AssetNames
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
  param([string]$Message)
  Write-Host ("[{0}] {1}" -f (Get-Date -Format "HH:mm:ss"), $Message)
}

if (-not $Tag) {
  $Tag = "v$Version"
}
if (-not $Title) {
  $Title = "Project Aura v$Version"
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
if (-not $AssetsDir) {
  $AssetsDir = Join-Path $root ("release-assets\{0}" -f $Tag)
}
if (-not $NotesPath) {
  $NotesPath = Join-Path $root ("docs\releases\v{0}.md" -f $Version)
}

if (-not (Test-Path $AssetsDir)) {
  throw "Assets directory not found: $AssetsDir"
}

if (-not $AssetNames -or $AssetNames.Count -eq 0) {
  $AssetNames = @(
    ("project_aura_{0}_ota_firmware.bin" -f $Version)
  )
}

foreach ($name in $AssetNames) {
  $path = Join-Path $AssetsDir $name
  if (-not (Test-Path $path)) {
    throw "Missing asset file: $path"
  }
}

if (-not (Get-Command curl.exe -ErrorAction SilentlyContinue)) {
  throw "curl.exe is required but not found."
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
  throw "git is required but not found."
}

function Get-BasicAuthValue {
  $request = "protocol=https`nhost=github.com`nusername=21cncstudio`n`n"
  $oldGcmInteractive = $env:GCM_INTERACTIVE
  try {
    $env:GCM_INTERACTIVE = "never"
    $raw = $request | git credential fill 2>$null
  } finally {
    if ($null -eq $oldGcmInteractive) {
      Remove-Item Env:\GCM_INTERACTIVE -ErrorAction SilentlyContinue
    } else {
      $env:GCM_INTERACTIVE = $oldGcmInteractive
    }
  }
  if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($raw)) {
    throw "Unable to read GitHub credentials from git credential helper."
  }

  $map = @{}
  foreach ($line in ($raw -split "`n")) {
    if ($line -match "=") {
      $k, $v = $line.Split("=", 2)
      $map[$k.Trim()] = $v.Trim()
    }
  }

  if (-not $map.ContainsKey("username") -or -not $map.ContainsKey("password")) {
    throw "Credential helper returned incomplete GitHub credentials."
  }

  return [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(("{0}:{1}" -f $map["username"], $map["password"])))
}

function Invoke-CurlJson {
  param(
    [string]$Method,
    [string]$Url,
    [string]$BasicAuth,
    [object]$BodyObj,
    [int]$TimeoutSec = 60
  )

  $curlArgs = @(
    "-sS",
    "--fail-with-body",
    "--http1.1",
    "--connect-timeout", "$ConnectTimeoutSec",
    "--max-time", "$TimeoutSec",
    "-X", $Method,
    $Url,
    "-H", ("Authorization: Basic {0}" -f $BasicAuth),
    "-H", "Accept: application/vnd.github+json",
    "-H", "X-GitHub-Api-Version: 2022-11-28",
    "-H", "User-Agent: project-aura-release-script",
    "-H", "Expect:"
  )

  $tmp = $null
  try {
    if ($null -ne $BodyObj) {
      $tmp = Join-Path $env:TEMP "aura_release_payload.json"
      $json = $BodyObj | ConvertTo-Json -Depth 20
      Set-Content -Path $tmp -Value $json -Encoding UTF8
      $curlArgs += @("-H", "Content-Type: application/json", "--data-binary", ("@{0}" -f $tmp))
    }

    $response = & curl.exe @curlArgs
    if ($LASTEXITCODE -ne 0) {
      throw "curl failed with exit code $LASTEXITCODE"
    }
  } finally {
    if ($tmp -and (Test-Path $tmp)) {
      Remove-Item -Force $tmp -ErrorAction SilentlyContinue
    }
  }

  if ([string]::IsNullOrWhiteSpace($response)) {
    return $null
  }
  return ($response | ConvertFrom-Json)
}

function Invoke-CurlUpload {
  param(
    [string]$Url,
    [string]$FilePath,
    [string]$BasicAuth,
    [int]$TimeoutSec = 300
  )

  $curlArgs = @(
    "-sS",
    "--fail-with-body",
    "--http1.1",
    "--connect-timeout", "$ConnectTimeoutSec",
    "--max-time", "$TimeoutSec",
    "-X", "POST",
    $Url,
    "-H", ("Authorization: Basic {0}" -f $BasicAuth),
    "-H", "Accept: application/vnd.github+json",
    "-H", "X-GitHub-Api-Version: 2022-11-28",
    "-H", "User-Agent: project-aura-release-script",
    "-H", "Content-Type: application/octet-stream",
    "-H", "Expect:",
    "--data-binary", ("@{0}" -f $FilePath)
  )

  $response = & curl.exe @curlArgs
  if ($LASTEXITCODE -ne 0) {
    throw "Upload failed for $FilePath (exit code $LASTEXITCODE)"
  }
  return ($response | ConvertFrom-Json)
}

Write-Step "Resolving GitHub credentials"
$basicAuth = Get-BasicAuthValue
$headers = @{
  Authorization = "Basic $basicAuth"
  Accept = "application/vnd.github+json"
  "X-GitHub-Api-Version" = "2022-11-28"
  "User-Agent" = "project-aura-release-script"
}

$release = $null
$releaseByTagUrl = "https://api.github.com/repos/$Repo/releases/tags/$Tag"
Write-Step "Checking existing release by tag: $Tag"
try {
  $release = Invoke-RestMethod -Method Get -Uri $releaseByTagUrl -Headers $headers -TimeoutSec $ApiTimeoutSec
} catch {
  $status = $null
  if ($_.Exception.Response) {
    try { $status = [int]$_.Exception.Response.StatusCode } catch {}
  }
  if ($status -ne 404) {
    throw
  }
}

$notesText = $null
if (-not $SkipReleaseUpdate -and -not $SkipNotes -and (Test-Path $NotesPath)) {
  Write-Step "Loading release notes: $NotesPath"
  $notesText = Get-Content -Raw -Path $NotesPath
}

if (-not $release) {
  Write-Step "Creating release $Tag (target: $TargetCommitish)"
  $createPayload = @{
    tag_name = $Tag
    target_commitish = $TargetCommitish
    name = $Title
    draft = [bool]$Draft
    prerelease = $false
    generate_release_notes = $false
  }
  if ($notesText) {
    $createPayload["body"] = $notesText
  }
  $release = Invoke-CurlJson -Method "POST" -Url ("https://api.github.com/repos/$Repo/releases") -BasicAuth $basicAuth -BodyObj $createPayload -TimeoutSec $ApiTimeoutSec
  Write-Host "Created release: $($release.html_url)"
} else {
  if ($SkipReleaseUpdate) {
    Write-Step "Skipping release metadata update for existing release id=$($release.id)"
    Write-Host "Using release: $($release.html_url)"
  } else {
    Write-Step "Updating existing release id=$($release.id)"
    $patchPayload = @{
      name = $Title
      draft = [bool]$Draft
      prerelease = $false
    }
    if ($notesText) {
      $patchPayload["body"] = $notesText
    }
    $release = Invoke-CurlJson -Method "PATCH" -Url ("https://api.github.com/repos/$Repo/releases/$($release.id)") -BasicAuth $basicAuth -BodyObj $patchPayload -TimeoutSec $ApiTimeoutSec
    Write-Host "Updated release: $($release.html_url)"
  }
}

$assetsApiUrl = "https://api.github.com/repos/$Repo/releases/$($release.id)/assets"
Write-Step "Fetching existing assets"
$existing = Invoke-RestMethod -Method Get -Uri $assetsApiUrl -Headers $headers -TimeoutSec $ApiTimeoutSec
if ($null -eq $existing) {
  $existing = @()
} elseif ($existing -isnot [System.Array]) {
  $existing = @($existing)
}

if ($PruneAssetsToList) {
  Write-Step "Pruning release assets to requested list"
  foreach ($asset in $existing) {
    if ($AssetNames -notcontains $asset.name) {
      $deleteUrl = "https://api.github.com/repos/$Repo/releases/assets/$($asset.id)"
      [void](Invoke-CurlJson -Method "DELETE" -Url $deleteUrl -BasicAuth $basicAuth -BodyObj $null -TimeoutSec $ApiTimeoutSec)
      Write-Host "Deleted extra asset: $($asset.name)"
    }
  }

  $existing = Invoke-RestMethod -Method Get -Uri $assetsApiUrl -Headers $headers -TimeoutSec $ApiTimeoutSec
  if ($null -eq $existing) {
    $existing = @()
  } elseif ($existing -isnot [System.Array]) {
    $existing = @($existing)
  }
}

foreach ($name in $AssetNames) {
  $path = Join-Path $AssetsDir $name
  Write-Step "Uploading asset: $name"
  $dupes = $existing | Where-Object { $_.name -eq $name }
  foreach ($asset in $dupes) {
    $deleteUrl = "https://api.github.com/repos/$Repo/releases/assets/$($asset.id)"
    [void](Invoke-CurlJson -Method "DELETE" -Url $deleteUrl -BasicAuth $basicAuth -BodyObj $null -TimeoutSec $ApiTimeoutSec)
    Write-Host "Deleted old asset: $name"
  }

  $uploadUrl = "https://uploads.github.com/repos/$Repo/releases/$($release.id)/assets?name=$([uri]::EscapeDataString($name))"
  $uploaded = $null
  $maxRetries = 3
  for ($attempt = 1; $attempt -le $maxRetries; $attempt++) {
    try {
      Write-Step ("Attempt {0}/{1}: {2}" -f $attempt, $maxRetries, $name)
      $uploaded = Invoke-CurlUpload -Url $uploadUrl -FilePath $path -BasicAuth $basicAuth -TimeoutSec $UploadTimeoutSec
      break
    } catch {
      if ($attempt -eq $maxRetries) {
        throw
      }
      Start-Sleep -Seconds (2 * $attempt)
    }
  }

  Write-Host ("Uploaded: {0} ({1} bytes)" -f $uploaded.name, $uploaded.size)
}

Write-Host ""
Write-Host "Release ready: $($release.html_url)"
