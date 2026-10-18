param(
    [Parameter(Mandatory = $true)]
    [string]$GameDirectory,
    [string]$Executable = "$(Join-Path $PSScriptRoot '..\build\Release\Win32\ThiefRunner\ThiefRunner.exe')"
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$testRoot = Join-Path $repoRoot '.codex-build\mission-load-regression'
New-Item -ItemType Directory -Force -Path $testRoot | Out-Null

function Add-U32([System.Collections.Generic.List[byte]]$bytes, [uint32]$value) {
    $bytes.AddRange([BitConverter]::GetBytes($value))
}

function Add-Tag([System.Collections.Generic.List[byte]]$bytes, [string]$value) {
    $tag = [byte[]]::new(12)
    $encoded = [Text.Encoding]::ASCII.GetBytes($value)
    [Array]::Copy($encoded, $tag, [Math]::Min($encoded.Length, 11))
    $bytes.AddRange($tag)
}

# A structurally valid tag file whose required FILE_TYPE payload is only two
# bytes.  It reaches the normal loading-panel -> dbLoadTagFile -> decoder path.
$truncated = [System.Collections.Generic.List[byte]]::new()
$tableOffset = [uint32](272 + 24 + 2)
Add-U32 $truncated $tableOffset
Add-U32 $truncated 0
Add-U32 $truncated 1
$truncated.AddRange([byte[]]::new(256))
$truncated.AddRange([byte[]](0xDE, 0xAD, 0xBE, 0xEF))
Add-Tag $truncated 'FILE_TYPE'
Add-U32 $truncated 0
Add-U32 $truncated 1
Add-U32 $truncated 0
$truncated.AddRange([byte[]](0x01, 0x00))
Add-U32 $truncated 1
Add-Tag $truncated 'FILE_TYPE'
Add-U32 $truncated 272
Add-U32 $truncated 2
$truncatedPath = Join-Path $testRoot 'truncated-file-type.mis'
[IO.File]::WriteAllBytes($truncatedPath, $truncated.ToArray())

# A container with a valid signature but an impossible directory count.  This
# covers the pre-decoder open boundary without using or copying retail data.
$badDirectory = [System.Collections.Generic.List[byte]]::new()
Add-U32 $badDirectory 272
Add-U32 $badDirectory 0
Add-U32 $badDirectory 1
$badDirectory.AddRange([byte[]]::new(256))
$badDirectory.AddRange([byte[]](0xDE, 0xAD, 0xBE, 0xEF))
Add-U32 $badDirectory ([uint32]::MaxValue)
$badDirectoryPath = Join-Path $testRoot 'impossible-directory.mis'
[IO.File]::WriteAllBytes($badDirectoryPath, $badDirectory.ToArray())

$cases = @($truncatedPath, $badDirectoryPath)
foreach ($case in $cases) {
    $diagnostics = Join-Path $testRoot (([IO.Path]::GetFileNameWithoutExtension($case)) + '.log')
    $oldDiagnostics = $env:DARK_DIAGNOSTICS_PATH
    try {
        $env:DARK_DIAGNOSTICS_PATH = $diagnostics
        $process = Start-Process -FilePath (Resolve-Path $Executable).Path `
            -WorkingDirectory (Resolve-Path $GameDirectory).Path `
            -ArgumentList @('mission_loop_test=1',
                "mission_loop_test_file=$case",
                'mission_loop_test_expect_failure=1') -PassThru
        if (-not $process.WaitForExit(30000)) {
            Stop-Process -Id $process.Id
            throw "Timed out waiting for mission rejection: $case"
        }
        if (-not (Test-Path $diagnostics) -or
            -not (Select-String -Quiet -LiteralPath $diagnostics `
                -Pattern 'mission-regression.*PASS:')) {
            throw "Mission rejection did not record PASS: $case"
        }
    }
    finally {
        $env:DARK_DIAGNOSTICS_PATH = $oldDiagnostics
    }
}

Write-Host "PASS: $($cases.Count) malformed missions were rejected through the normal mission-loading path."
