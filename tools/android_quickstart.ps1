[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Mission,

    [string]$Position,
    [string]$Angles,
    [ValidateSet('configured', 'vanilla')]
    [string]$ContentMode,
    [string]$FanMission,
    [string]$TextureAudit,
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
$packageName = 'io.github.inkmakerdroid.darkengine'

function ConvertTo-AdbShellArgument([string]$Value) {
    # adb joins arguments into a command interpreted by the device shell.
    # Preserve spaces and shell metacharacters in string extras such as FM
    # directory names, escaping embedded single quotes in the usual sh form.
    $singleQuote = [string][char]39
    $doubleQuote = [string][char]34
    $escapedQuote = $singleQuote + $doubleQuote + $singleQuote +
        $doubleQuote + $singleQuote
    return $singleQuote + $Value.Replace($singleQuote, $escapedQuote) +
        $singleQuote
}

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
    '--es', "${packageName}.extra.QUICKSTART", $Mission
)
if ($ContentMode) {
    $launchArguments += @(
        '--es', "${packageName}.extra.CONTENT", $ContentMode)
}
if ($FanMission) {
    if (!$ContentMode) {
        $launchArguments += @(
            '--es', "${packageName}.extra.CONTENT", 'configured')
    }
    $launchArguments += @(
        '--es', "${packageName}.extra.FAN_MISSION",
        (ConvertTo-AdbShellArgument $FanMission))
}
if ($Position) {
    $launchArguments += @(
        '--es', "${packageName}.extra.QUICKSTART_POS", $Position)
}
if ($Angles) {
    $launchArguments += @(
        '--es', "${packageName}.extra.QUICKSTART_ANGLES", $Angles)
}
if ($TextureAudit) {
    $launchArguments += @(
        '--es', "${packageName}.extra.TEXTURE_AUDIT", $TextureAudit)
}
if ($AutoStop -gt 0) {
    $launchArguments += @(
        '--ei', "${packageName}.extra.AUTO_STOP", "$AutoStop")
}

& $adbPath @deviceArguments @launchArguments
if ($LASTEXITCODE -ne 0) {
    throw "Android quick-start failed with exit code $LASTEXITCODE."
}

Write-Host "Started $Mission directly on $packageName."
