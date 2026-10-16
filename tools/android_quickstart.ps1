[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Mission,

    [string]$Position,
    [string]$Angles,
    [ValidateRange(0, 2147483647)]
    [int]$AutoStop = 0,
    [string]$Serial,
    [switch]$Build,
    [switch]$Install
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repositoryRoot 'android'
$apkPath = Join-Path $androidRoot 'app\build\outputs\apk\debug\app-debug.apk'
$packageName = 'org.opendarkengine.thief2'

$adbCommand = Get-Command adb -ErrorAction SilentlyContinue
if ($adbCommand) {
    $adbPath = $adbCommand.Source
} else {
    $adbPath = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
    if (!(Test-Path -LiteralPath $adbPath)) {
        throw 'adb was not found on PATH or in the default Android SDK location.'
    }
}

if ($Build) {
    Push-Location $androidRoot
    try {
        & .\gradlew.bat assembleDebug
        if ($LASTEXITCODE -ne 0) {
            throw "Android build failed with exit code $LASTEXITCODE."
        }
    } finally {
        Pop-Location
    }
}

$deviceArguments = @()
if ($Serial) {
    $deviceArguments += @('-s', $Serial)
}

if ($Install) {
    if (!(Test-Path -LiteralPath $apkPath)) {
        throw "Debug APK not found at $apkPath. Use -Build first."
    }
    & $adbPath @deviceArguments install -r $apkPath
    if ($LASTEXITCODE -ne 0) {
        throw "APK installation failed with exit code $LASTEXITCODE."
    }
}

$launchArguments = @(
    'shell', 'am', 'start', '-S', '-W',
    '-n', "$packageName/.LauncherActivity",
    '--es', 'org.opendarkengine.thief2.extra.QUICKSTART', $Mission
)
if ($Position) {
    $launchArguments += @(
        '--es', 'org.opendarkengine.thief2.extra.QUICKSTART_POS', $Position)
}
if ($Angles) {
    $launchArguments += @(
        '--es', 'org.opendarkengine.thief2.extra.QUICKSTART_ANGLES', $Angles)
}
if ($AutoStop -gt 0) {
    $launchArguments += @(
        '--ei', 'org.opendarkengine.thief2.extra.AUTO_STOP', "$AutoStop")
}

& $adbPath @deviceArguments @launchArguments
if ($LASTEXITCODE -ne 0) {
    throw "Android quick-start failed with exit code $LASTEXITCODE."
}

Write-Host "Started $Mission directly on $packageName."
