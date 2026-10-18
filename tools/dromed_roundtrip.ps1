[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$GameRoot,

    [Parameter(Mandatory = $true)]
    [string]$FanMission,

    [string]$Mission,
    [string]$GameSys,
    [string]$DromEd,
    [string]$OutputRoot,
    [int]$TimeoutSeconds = 240
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DromEd) {
    $DromEd = Join-Path $repoRoot 'build\Release\Win32\DromEd\DromEd.exe'
}
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $repoRoot '.codex-build\dromed-roundtrip'
}

$game = (Resolve-Path -LiteralPath $GameRoot).Path
$editor = (Resolve-Path -LiteralPath $DromEd).Path
$fmRoot = Join-Path (Join-Path $game 'FMs') $FanMission
$fmRoot = (Resolve-Path -LiteralPath $fmRoot).Path

if (-not $Mission) {
    $missions = @(Get-ChildItem -LiteralPath $fmRoot -Filter '*.mis' -File)
    if ($missions.Count -ne 1) {
        throw "Specify -Mission: the FM contains $($missions.Count) .mis files."
    }
    $Mission = $missions[0].Name
}
$sourceMission = Join-Path $fmRoot $Mission
$sourceMission = (Resolve-Path -LiteralPath $sourceMission).Path

if (-not $GameSys) {
    $gameSystems = @(Get-ChildItem -LiteralPath $fmRoot -Filter '*.gam' -File)
    if ($gameSystems.Count -eq 1) {
        $GameSys = $gameSystems[0].Name
    }
}
$sourceGameSys = $null
if ($GameSys) {
    $sourceGameSys = (Resolve-Path -LiteralPath (Join-Path $fmRoot $GameSys)).Path
}

$safeName = ($FanMission -replace '[^A-Za-z0-9._-]', '_').Trim('_')
if ($safeName.Length -gt 64) { $safeName = $safeName.Substring(0, 64) }
$caseRoot = Join-Path $OutputRoot $safeName
New-Item -ItemType Directory -Force -Path $caseRoot | Out-Null
$caseRoot = (Resolve-Path -LiteralPath $caseRoot).Path

$firstMission = Join-Path $caseRoot 'first.mis'
$secondMission = Join-Path $caseRoot 'reloaded.mis'
$firstGameSys = Join-Path $caseRoot 'first.gam'
$secondGameSys = Join-Path $caseRoot 'reloaded.gam'
$cycleCommands = Join-Path $caseRoot 'initial-save.cmd'
$missionReloadCommands = Join-Path $caseRoot 'mission-reload-save.cmd'
$gameSysReloadCommands = Join-Path $caseRoot 'gamesys-reload-save.cmd'
$verifyCommands = Join-Path $caseRoot 'verify.cmd'
$diagnostics = Join-Path $caseRoot 'darkengine-diagnostics.log'
$crashes = Join-Path $caseRoot 'crashes'
New-Item -ItemType Directory -Force -Path $crashes | Out-Null

@(
    'db_write_profile preserve'
    'db_format_report'
    "save_mission $firstMission"
    "save_gamesys $firstGameSys"
    'quit_game'
) | Set-Content -LiteralPath $cycleCommands -Encoding ASCII
@(
    'db_write_profile preserve'
    "save_mission $secondMission"
    'quit_game'
) | Set-Content -LiteralPath $missionReloadCommands -Encoding ASCII
@(
    'db_write_profile preserve'
    "load_gamesys $firstGameSys"
    "save_gamesys $secondGameSys"
    'quit_game'
) | Set-Content -LiteralPath $gameSysReloadCommands -Encoding ASCII
@(
    'db_format_report'
    'quit_game'
) | Set-Content -LiteralPath $verifyCommands -Encoding ASCII

function ConvertTo-NativeArgument([string]$Value) {
    if ($Value -notmatch '[\s"]') { return $Value }
    return '"' + ($Value -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
}

function Invoke-DromEd([string[]]$Arguments, [string]$DiagnosticPath) {
    $oldDiagnostics = [Environment]::GetEnvironmentVariable('DARK_DIAGNOSTICS_PATH')
    $oldCrashOutput = [Environment]::GetEnvironmentVariable('DARK_CRASH_OUTPUT_DIR')
    $oldAssertAction = [Environment]::GetEnvironmentVariable('DARK_ASSERT_ACTION')
    try {
        [Environment]::SetEnvironmentVariable('DARK_DIAGNOSTICS_PATH', $DiagnosticPath)
        [Environment]::SetEnvironmentVariable('DARK_CRASH_OUTPUT_DIR', $crashes)
        [Environment]::SetEnvironmentVariable('DARK_ASSERT_ACTION', 'ignore')
        $start = New-Object System.Diagnostics.ProcessStartInfo
        $start.FileName = $editor
        $start.WorkingDirectory = $game
        $start.UseShellExecute = $false
        $start.Arguments = (($Arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' ')
        $process = New-Object System.Diagnostics.Process
        $process.StartInfo = $start
        if (-not $process.Start()) { throw 'DromEd did not start.' }
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "DromEd PID $($process.Id) exceeded the $TimeoutSeconds second timeout."
        }
        return $process.ExitCode
    }
    finally {
        [Environment]::SetEnvironmentVariable('DARK_DIAGNOSTICS_PATH', $oldDiagnostics)
        [Environment]::SetEnvironmentVariable('DARK_CRASH_OUTPUT_DIR', $oldCrashOutput)
        [Environment]::SetEnvironmentVariable('DARK_ASSERT_ACTION', $oldAssertAction)
    }
}

function Get-RangeHash([System.IO.FileStream]$Stream, [long]$Offset, [long]$Length) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $buffer = New-Object byte[] (1024 * 1024)
    $Stream.Position = $Offset
    $remaining = $Length
    while ($remaining -gt 0) {
        $wanted = [Math]::Min([long]$buffer.Length, $remaining)
        $read = $Stream.Read($buffer, 0, [int]$wanted)
        if ($read -le 0) { throw 'Unexpected end of tag file while hashing a chunk.' }
        [void]$sha.TransformBlock($buffer, 0, $read, $buffer, 0)
        $remaining -= $read
    }
    [void]$sha.TransformFinalBlock((New-Object byte[] 0), 0, 0)
    return ([BitConverter]::ToString($sha.Hash) -replace '-', '').ToLowerInvariant()
}

function Get-TagInventory([string]$Path) {
    $stream = [System.IO.File]::Open($Path, 'Open', 'Read', 'ReadWrite')
    $reader = New-Object System.IO.BinaryReader($stream)
    try {
        if ($stream.Length -lt 272) { throw "$Path is too small to be a tag file." }
        $tableOffset = $reader.ReadUInt32()
        [void]$reader.ReadUInt32()
        [void]$reader.ReadUInt32()
        if ($tableOffset -ge $stream.Length) { throw "$Path has an invalid tag table offset." }
        $stream.Position = $tableOffset
        $count = $reader.ReadUInt32()
        $entries = @()
        for ($index = 0; $index -lt $count; ++$index) {
            $labelBytes = $reader.ReadBytes(12)
            if ($labelBytes.Length -ne 12) { throw "$Path has a truncated tag table." }
            $zero = [Array]::IndexOf($labelBytes, [byte]0)
            if ($zero -lt 0) { $zero = 12 }
            $label = [Text.Encoding]::ASCII.GetString($labelBytes, 0, $zero)
            $offset = $reader.ReadUInt32()
            $size = $reader.ReadInt32()
            if ($size -lt 0 -or ([long]$offset + 24 + $size) -gt $stream.Length) {
                throw "$Path has an invalid $label chunk extent."
            }
            $returnPosition = $stream.Position
            $stream.Position = $offset + 12
            $major = $reader.ReadUInt32()
            $minor = $reader.ReadUInt32()
            # Historical writers leave the block header's size field at zero;
            # the table of contents is authoritative.
            [void]$reader.ReadUInt32()
            $hash = Get-RangeHash $stream ([long]$offset + 24) $size
            $stream.Position = $returnPosition
            $entries += [pscustomobject]@{
                Tag = $label
                Major = $major
                Minor = $minor
                Size = $size
                Sha256 = $hash
            }
        }
        return @($entries | Sort-Object Tag)
    }
    finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}

function Compare-TagInventory([string]$Label, [object[]]$Before, [object[]]$After) {
    $beforeByTag = @{}
    $afterByTag = @{}
    foreach ($entry in $Before) { $beforeByTag[$entry.Tag] = $entry }
    foreach ($entry in $After) { $afterByTag[$entry.Tag] = $entry }
    $missing = @($beforeByTag.Keys | Where-Object { -not $afterByTag.ContainsKey($_) } | Sort-Object)
    $added = @($afterByTag.Keys | Where-Object { -not $beforeByTag.ContainsKey($_) } | Sort-Object)
    $versionChanged = @()
    $contentChanged = @()
    foreach ($tag in $beforeByTag.Keys) {
        if (-not $afterByTag.ContainsKey($tag)) { continue }
        $left = $beforeByTag[$tag]
        $right = $afterByTag[$tag]
        if ($left.Major -ne $right.Major -or $left.Minor -ne $right.Minor) {
            $versionChanged += $tag
        }
        if ($left.Size -ne $right.Size -or $left.Sha256 -ne $right.Sha256) {
            $contentChanged += $tag
        }
    }
    return [pscustomobject]@{
        Name = $Label
        BeforeChunks = $Before.Count
        AfterChunks = $After.Count
        Missing = $missing
        Added = $added
        VersionChanged = @($versionChanged | Sort-Object)
        ContentChanged = @($contentChanged | Sort-Object)
        StructurallyCompatible = ($missing.Count -eq 0 -and $versionChanged.Count -eq 0)
        ByteStable = ($missing.Count -eq 0 -and $added.Count -eq 0 -and
                      $versionChanged.Count -eq 0 -and $contentChanged.Count -eq 0)
    }
}

function Get-BrushInventory([string]$Path) {
    $stream = [System.IO.File]::Open($Path, 'Open', 'Read', 'ReadWrite')
    $reader = New-Object System.IO.BinaryReader($stream)
    try {
        $tableOffset = $reader.ReadUInt32()
        [void]$reader.ReadUInt32()
        [void]$reader.ReadUInt32()
        $stream.Position = $tableOffset
        $chunkCount = $reader.ReadUInt32()
        $brushOffset = $null
        $brushSize = 0
        for ($index = 0; $index -lt $chunkCount; ++$index) {
            $labelBytes = $reader.ReadBytes(12)
            $zero = [Array]::IndexOf($labelBytes, [byte]0)
            if ($zero -lt 0) { $zero = 12 }
            $label = [Text.Encoding]::ASCII.GetString($labelBytes, 0, $zero)
            $offset = $reader.ReadUInt32()
            $size = $reader.ReadInt32()
            if ($label -eq 'BRLIST') {
                $brushOffset = [long]$offset + 24
                $brushSize = $size
            }
        }
        if ($null -eq $brushOffset) { return $null }
        $stream.Position = $brushOffset
        $bytes = $reader.ReadBytes($brushSize)
        if ($bytes.Length -ne $brushSize) { throw "$Path has a truncated BRLIST chunk." }

        $counts = @(0, 0, 0, 0, 0, 0)
        $position = 0
        $maxFaces = 0
        while ($position -lt $bytes.Length) {
            # Packed BRLIST 1.0 records have a 76-byte fixed prefix. Terrain
            # records append one packed 10-byte TexInfo for every face.
            if ($position + 76 -gt $bytes.Length) {
                throw "$Path has a truncated BRLIST record at byte $position."
            }
            $media = [int]$bytes[$position + 10]
            if ($media -ge 128) { $media -= 256 }
            $type = if ($media -ge 0) { 0 } else { -$media }
            if ($type -gt 5) { throw "$Path has invalid BRLIST media $media at byte $position." }
            $faces = [int]$bytes[$position + 67]
            $recordSize = 76
            if ($type -eq 0) {
                if ($faces -gt 28) { throw "$Path has a $faces-face terrain brush at byte $position." }
                $recordSize += 10 * $faces
                if ($faces -gt $maxFaces) { $maxFaces = $faces }
            }
            if ($position + $recordSize -gt $bytes.Length) {
                throw "$Path has a truncated BRLIST payload at byte $position."
            }
            ++$counts[$type]
            $position += $recordSize
        }
        return [pscustomobject]@{
            Path = $Path
            ChunkBytes = $brushSize
            Total = ($counts | Measure-Object -Sum).Sum
            Terrain = $counts[0]
            Light = $counts[1]
            Area = $counts[2]
            Object = $counts[3]
            Flow = $counts[4]
            Room = $counts[5]
            MaxTerrainFaces = $maxFaces
        }
    }
    finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}

function Compare-BrushInventory([string]$Label, $Before, $After) {
    $matches = ($null -ne $Before -and $null -ne $After)
    foreach ($field in @('Total','Terrain','Light','Area','Object','Flow','Room','MaxTerrainFaces')) {
        if ($matches -and $Before.$field -ne $After.$field) { $matches = $false }
    }
    return [pscustomobject]@{
        Name = $Label
        Before = $Before
        After = $After
        Matches = $matches
    }
}

$cycleExit = Invoke-DromEd @(
    "fm=$FanMission"
    "file=$sourceMission"
    "edit_script=$cycleCommands"
) $diagnostics
if ($cycleExit -ne 0) { throw "DromEd round-trip cycle exited with code $cycleExit." }

foreach ($required in @($firstMission, $firstGameSys)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "DromEd did not produce $required."
    }
}

$missionReloadDiagnostics = Join-Path $caseRoot 'mission-reload-diagnostics.log'
$missionReloadExit = Invoke-DromEd @(
    "fm=$FanMission"
    "file=$firstMission"
    "edit_script=$missionReloadCommands"
) $missionReloadDiagnostics
if ($missionReloadExit -ne 0) { throw "DromEd mission reload/save exited with code $missionReloadExit." }

$gameSysReloadDiagnostics = Join-Path $caseRoot 'gamesys-reload-diagnostics.log'
$gameSysReloadExit = Invoke-DromEd @(
    "fm=$FanMission"
    "edit_script=$gameSysReloadCommands"
) $gameSysReloadDiagnostics
if ($gameSysReloadExit -ne 0) { throw "DromEd gamesys reload/save exited with code $gameSysReloadExit." }

foreach ($required in @($secondMission, $secondGameSys)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "DromEd did not produce $required."
    }
}

$reloadDiagnostics = Join-Path $caseRoot 'reload-diagnostics.log'
$reloadExit = Invoke-DromEd @(
    "fm=$FanMission"
    "file=$secondMission"
    "edit_script=$verifyCommands"
) $reloadDiagnostics

$comparisons = @(
    Compare-TagInventory 'MIS source -> first save' (Get-TagInventory $sourceMission) (Get-TagInventory $firstMission)
    Compare-TagInventory 'MIS save -> reload/save' (Get-TagInventory $firstMission) (Get-TagInventory $secondMission)
    Compare-TagInventory 'GAM save -> reload/save' (Get-TagInventory $firstGameSys) (Get-TagInventory $secondGameSys)
)
if ($sourceGameSys) {
    $comparisons += Compare-TagInventory 'GAM source -> first save' (Get-TagInventory $sourceGameSys) (Get-TagInventory $firstGameSys)
}

$structuralPass = @($comparisons | Where-Object { -not $_.StructurallyCompatible }).Count -eq 0
$brushComparisons = @(
    Compare-BrushInventory 'MIS source -> first save' (Get-BrushInventory $sourceMission) (Get-BrushInventory $firstMission)
    Compare-BrushInventory 'MIS save -> reload/save' (Get-BrushInventory $firstMission) (Get-BrushInventory $secondMission)
)
$semanticPass = @($brushComparisons | Where-Object { -not $_.Matches }).Count -eq 0
$assertionLines = @()
foreach ($diagnosticFile in @($diagnostics, $missionReloadDiagnostics,
                               $gameSysReloadDiagnostics, $reloadDiagnostics)) {
    if (Test-Path -LiteralPath $diagnosticFile) {
        $assertionLines += @(Select-String -LiteralPath $diagnosticFile -Pattern '^.*ERROR assertion +Out of range message \(File:')
    }
}
$report = [pscustomobject]@{
    Generated = (Get-Date).ToString('o')
    GameRoot = $game
    FanMission = $FanMission
    SourceMission = $sourceMission
    SourceGameSys = $sourceGameSys
    WriteProfile = 'preserve'
    CycleExitCode = $cycleExit
    MissionReloadExitCode = $missionReloadExit
    GameSysReloadExitCode = $gameSysReloadExit
    ReloadExitCode = $reloadExit
    FullReloadPassed = ($missionReloadExit -eq 0 -and $gameSysReloadExit -eq 0 -and $reloadExit -eq 0)
    StructuralPass = $structuralPass
    SemanticPass = $semanticPass
    AssertionCount = $assertionLines.Count
    OverallPassed = ($missionReloadExit -eq 0 -and $gameSysReloadExit -eq 0 -and $reloadExit -eq 0 -and $structuralPass -and $semanticPass -and $assertionLines.Count -eq 0)
    Comparisons = $comparisons
    BrushComparisons = $brushComparisons
}
$reportPath = Join-Path $caseRoot 'roundtrip-report.json'
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath -Encoding UTF8

$report | Format-List Generated,FanMission,SourceMission,SourceGameSys,WriteProfile,CycleExitCode,MissionReloadExitCode,GameSysReloadExitCode,ReloadExitCode,FullReloadPassed,StructuralPass,SemanticPass,AssertionCount,OverallPassed
$comparisons | Format-Table Name,BeforeChunks,AfterChunks,StructurallyCompatible,ByteStable,@{Name='Changed';Expression={$_.ContentChanged.Count}} -AutoSize
$brushComparisons | Format-Table Name,Matches,@{Name='BeforeBrushes';Expression={$_.Before.Total}},@{Name='AfterBrushes';Expression={$_.After.Total}},@{Name='BeforeObjects';Expression={$_.Before.Object}},@{Name='AfterObjects';Expression={$_.After.Object}} -AutoSize
Write-Output "Report: $reportPath"

if (-not $report.OverallPassed) { exit 1 }
