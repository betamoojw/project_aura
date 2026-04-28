$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
Set-Location $repoRoot

$pioCmd = Get-Command pio -ErrorAction SilentlyContinue
if ($pioCmd) {
    $pioExe = $pioCmd.Source
} else {
    $pioExe = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
    if (-not (Test-Path $pioExe)) {
        Write-Error "PlatformIO not found. Install it or add 'pio' to PATH."
        exit 1
    }
}

if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Error "gcc not found. Install MSYS2 mingw-w64-x86_64-toolchain and add C:\\msys64\\mingw64\\bin to PATH."
    exit 1
}

function Invoke-PioTest {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Args
    )

    & $pioExe @Args
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Invoke-PioTest @("test", "-e", "native_test")
Invoke-PioTest @("test", "-e", "native_test_sfa30_driver", "-f", "test_sfa30_driver")
Invoke-PioTest @("test", "-e", "native_test_sfa40_driver", "-f", "test_sfa40_driver")
Invoke-PioTest @("test", "-e", "native_test_dfr_optional_gas_driver", "-f", "test_dfr_optional_gas_driver")

exit 0
