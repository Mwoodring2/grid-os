$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    py -m platformio --version
    if ($LASTEXITCODE -ne 0) { throw 'Install PlatformIO first: py -m pip install platformio' }
    py -m platformio run -e grid_es3c28p
    if ($LASTEXITCODE -ne 0) { throw 'GRID//OS build failed. Do not flash incomplete output.' }
    py scripts/verify_release.py
    if ($LASTEXITCODE -ne 0) { throw 'Release layout verification failed. Do not flash.' }
    $firmware = Join-Path $PSScriptRoot 'dist\GRID-OS-ES3C28P-v0.1.0-alpha.bin'
    if (-not (Test-Path -LiteralPath $firmware)) { throw 'Merged firmware was not generated.' }
    Get-FileHash -LiteralPath $firmware -Algorithm SHA256
    Write-Host "Firmware: $firmware"
    Write-Host 'Initial USB flash offset: 0x0000. Target: ES3C28P ESP32-S3 16 MB ONLY.'
} finally { Pop-Location }
