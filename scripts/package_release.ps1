[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir '..')
Set-Location $repoRoot

$appName = 'RanVisualizerCapture'
$buildDir = Join-Path $repoRoot 'build\package-win-x64'
$distRoot = Join-Path $repoRoot 'dist'
$stageDir = Join-Path $distRoot $appName
$zipPath = Join-Path $distRoot "$appName-win-x64.zip"

Write-Host "==> Packaging $appName (Release x64)"
Write-Host "==> Repo root: $repoRoot"

Write-Host "==> Configuring CMake (Visual Studio 2022, x64)"
cmake -S . -B $buildDir -G "Visual Studio 17 2022" -A x64

Write-Host "==> Building Release"
cmake --build $buildDir --config Release

Write-Host "==> Preparing dist staging directory"
if (Test-Path $stageDir) {
    Remove-Item -Recurse -Force $stageDir
}
New-Item -ItemType Directory -Path $stageDir -Force | Out-Null

Write-Host "==> Installing package contents"
cmake --install $buildDir --config Release --prefix $stageDir

if (!(Test-Path (Join-Path $stageDir "$appName.exe"))) {
    throw "Expected executable was not installed: $stageDir\\$appName.exe"
}

if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}

Write-Host "==> Creating zip archive"
Compress-Archive -Path (Join-Path $stageDir '*') -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "==> Package complete"
Write-Host "    Staging folder: $stageDir"
Write-Host "    Zip archive:    $zipPath"
