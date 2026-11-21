param(
    [ValidateSet("x64", "x86")]
    [string]$Architecture = "x64",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$CMakeArgs
)

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio Installer (vswhere.exe) was not found."
}

$candidates = @(
    @{ Range = "[18.0,19.0)"; Generator = "Visual Studio 18 2026"; Name = "Visual Studio 2026" },
    @{ Range = "[17.0,18.0)"; Generator = "Visual Studio 17 2022"; Name = "Visual Studio 2022" },
    @{ Range = "[16.0,17.0)"; Generator = "Visual Studio 16 2019"; Name = "Visual Studio 2019" }
)

$selected = $null
foreach ($candidate in $candidates) {
    $install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -version $candidate.Range -property installationPath
    if ($install) {
        $selected = $candidate
        $selected.Install = $install
        break
    }
}
if (-not $selected) {
    throw "No supported Visual Studio C++ toolchain was found."
}
if ($selected.Generator -ne "Visual Studio 18 2026") {
    Write-Warning "Visual Studio 2026 was not found; falling back to $($selected.Name)."
}

$cmake = Join-Path $selected.Install "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path -LiteralPath $cmake)) {
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
}
$platform = if ($Architecture -eq "x64") { "x64" } else { "Win32" }
$binaryDir = if ($Architecture -eq "x64") { "build-win64" } else { "build-win32" }
$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = [IO.Path]::GetFullPath((Join-Path $sourceDir $binaryDir))
$cache = Join-Path $buildPath "CMakeCache.txt"
if (Test-Path -LiteralPath $cache) {
    $cachedGenerator = Get-Content -LiteralPath $cache | Select-String `
        '^CMAKE_GENERATOR:INTERNAL=(.+)$' | ForEach-Object { $_.Matches[0].Groups[1].Value }
    if ($cachedGenerator -and $cachedGenerator -ne $selected.Generator) {
        Write-Warning "Clearing stale CMake metadata for $cachedGenerator."
        Remove-Item -LiteralPath $cache -Force
        $cmakeFiles = Join-Path $buildPath "CMakeFiles"
        if (Test-Path -LiteralPath $cmakeFiles) {
            Remove-Item -LiteralPath $cmakeFiles -Recurse -Force
        }
    }
}

& $cmake -S $sourceDir -B $buildPath -G $selected.Generator -A $platform `
    -DDARK_BUILD_DROMED=ON @CMakeArgs
exit $LASTEXITCODE
