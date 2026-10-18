[CmdletBinding()]
param(
    [ValidateSet('All', 'Game', 'DromEd')]
    [string]$Target = 'All'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dependencyRoot = [System.IO.Path]::GetFullPath(
    (Join-Path $repositoryRoot '..\..\darkengine-deps'))

function Install-GitDependency {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Repository,
        [Parameter(Mandatory = $true)][string]$Tag,
        [Parameter(Mandatory = $true)][string]$RequiredFile
    )

    $destination = Join-Path $dependencyRoot $Name
    $marker = Join-Path $destination $RequiredFile
    if (Test-Path -LiteralPath $marker -PathType Leaf) {
        Write-Host "$Name is already available at $destination"
        return
    }
    if (Test-Path -LiteralPath $destination) {
        throw "$destination exists but does not contain $RequiredFile. Move or repair it before retrying."
    }

    New-Item -ItemType Directory -Force -Path $dependencyRoot | Out-Null
    & git clone --branch $Tag --depth 1 $Repository $destination
    if ($LASTEXITCODE -ne 0 -or
        -not (Test-Path -LiteralPath $marker -PathType Leaf)) {
        throw "Failed to install $Name $Tag at $destination"
    }
}

if ($Target -in @('All', 'Game')) {
    Install-GitDependency `
        -Name 'squirrel' `
        -Repository 'https://github.com/albertodemichelis/squirrel.git' `
        -Tag 'v3.2' `
        -RequiredFile 'include\squirrel.h'
}

if ($Target -in @('All', 'DromEd')) {
    Install-GitDependency `
        -Name 'kctools' `
        -Repository 'https://github.com/JarrodDoyle/KCTools.git' `
        -Tag 'v1.3.4' `
        -RequiredFile 'KCTools.slnx'
}
