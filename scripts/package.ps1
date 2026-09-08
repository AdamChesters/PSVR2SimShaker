param(
    [string]$BuildDir = 'build',
    [string]$Configuration = 'Release',
    [string]$InnoCompiler = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$packageRoot = Join-Path $projectRoot 'dist\package'
cmake --install (Join-Path $projectRoot $BuildDir) --config $Configuration --prefix $packageRoot
if ($LASTEXITCODE -ne 0) { throw 'Package staging failed' }
$archive = Join-Path $projectRoot 'dist\PSVR2SimShaker-0.1.1-Windows-x64.zip'
Compress-Archive -Path (Join-Path $packageRoot '*') -DestinationPath $archive -Force
if (-not (Test-Path -LiteralPath $InnoCompiler)) { throw 'Set -InnoCompiler to the installed ISCC.exe path. The portable ZIP is ready.' }
& $InnoCompiler (Join-Path $projectRoot 'installer\PSVR2SimShaker.iss')
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed' }
Get-FileHash -Algorithm SHA256 -LiteralPath $archive,(Join-Path $projectRoot 'dist\PSVR2SimShaker-0.1.1-Setup.exe') | ForEach-Object {
    "$($_.Hash.ToLowerInvariant())  $([IO.Path]::GetFileName($_.Path))"
} | Set-Content -Encoding utf8 (Join-Path $projectRoot 'dist\SHA256SUMS.txt')
