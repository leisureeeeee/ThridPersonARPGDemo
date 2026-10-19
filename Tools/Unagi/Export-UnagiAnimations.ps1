[CmdletBinding()]
param(
    [string]$MaxBatch,
    [string]$OutputDirectory,
    [switch]$IncludeAlternateScenes,
    [string]$Scene = 'Unagi_Idle',
    [switch]$AllScenes,
    [switch]$ImportIntoUE,
    [string]$EngineRoot,
    [string]$Destination = '/Game/Asset/Unagi/Animations/FromMax'
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$resource = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Content') -Directory -Filter '039-*')
if ($resource.Count -ne 1) { throw 'Expected exactly one Content/039-* resource folder.' }
$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $resource[0].FullName 'max') -Recurse -File -Filter 'Unagi_*.max' |
    Where-Object { $_.BaseName -notlike 'Unagi_Ghost*' })
if (-not $IncludeAlternateScenes) {
    $sourceFiles = @($sourceFiles | Where-Object { $_.Directory.Name -ceq 'MAX' })
}
if (-not $AllScenes) { $sourceFiles = @($sourceFiles | Where-Object { $_.BaseName -eq $Scene }) }
if ($sourceFiles.Count -eq 0) { throw 'No matching main-character animation scenes found.' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot 'Saved/UnagiMaxExport' }
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
# Never place generated files beside source scenes or overwrite an old export.
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $resource[0].FullName 'max')).TrimEnd('\')
if ($OutputDirectory.Equals($sourceRoot, [StringComparison]::OrdinalIgnoreCase) -or
    $OutputDirectory.StartsWith($sourceRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputDirectory must be outside the MAX source directory.'
}
$jobs = @($sourceFiles | Sort-Object FullName | ForEach-Object {
    $variant = if ($_.Directory.Name -ceq 'MAX') { 'Primary' } else { 'Alternate' }
    @{
        source = $_.FullName
        output = Join-Path (Join-Path $OutputDirectory $variant) ($_.BaseName + '.fbx')
    }
})
$workDirectory = Join-Path $projectRoot ('Saved/UnagiAnimationWork/' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $workDirectory | Out-Null
$reportFile = Join-Path $workDirectory 'export-report.json'
$configFile = Join-Path $workDirectory 'export-config.json'
[IO.File]::WriteAllText($configFile, (@{ jobs = $jobs; report_path = $reportFile } | ConvertTo-Json -Depth 6), [Text.UTF8Encoding]::new($false))
Write-Output "Export plan: $configFile ($($jobs.Count) scenes)"
if (-not $MaxBatch) {
    $command = Get-Command 3dsmaxbatch.exe -ErrorAction SilentlyContinue
    if ($command) { $MaxBatch = $command.Source }
    if (-not $MaxBatch) {
        $candidates = @(Get-ChildItem -Path 'C:/Program Files/Autodesk/3ds Max */3dsmaxbatch.exe' -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending)
        if ($candidates.Count -gt 0) { $MaxBatch = $candidates[0].FullName }
    }
}
if (-not $MaxBatch -or -not (Test-Path -LiteralPath $MaxBatch -PathType Leaf)) {
    [IO.File]::WriteAllText($reportFile, (@{
        status = 'blocked'
        reason = '3dsmaxbatch.exe not found. A licensed 3ds Max installation with FBX exporter and pymxs is required.'
        planned_scenes = $jobs.Count
    } | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
    throw "3ds Max Batch not found. Supply -MaxBatch after installing/licensing Max. Plan retained at $configFile"
}
$previousConfig = $env:UNAGI_EXPORT_CONFIG
try {
    $env:UNAGI_EXPORT_CONFIG = $configFile
    # A new batch process owns all resets/loads; no open editor is closed or reused.
    & $MaxBatch (Join-Path $PSScriptRoot 'export_unagi_animations.py') -v 3 -listenerlog (Join-Path $workDirectory 'max-listener.log') -log (Join-Path $workDirectory 'max.log')
    $maxExitCode = $LASTEXITCODE
    if ($maxExitCode -ne 0) { throw "Max batch failed (exit $maxExitCode); inspect $workDirectory" }
    if (-not (Test-Path -LiteralPath $reportFile)) { throw 'Max did not write an export report.' }
    $report = Get-Content -LiteralPath $reportFile -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($report.status -ne 'complete') { throw "MAX export incomplete: $reportFile" }
    Write-Output "Export report: $reportFile"
    if ($ImportIntoUE) {
        & (Join-Path $PSScriptRoot 'Import-UnagiAnimations.ps1') -EngineRoot $EngineRoot -SourceDirectory $OutputDirectory -Destination $Destination
    }
}
finally {
    $env:UNAGI_EXPORT_CONFIG = $previousConfig
}
