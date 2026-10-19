[CmdletBinding()]
param(
    [string]$EngineRoot,
    [string]$SourceDirectory,
    [string]$Mesh = '/Game/Asset/Unagi/SK_Unagi',
    [string]$Destination = '/Game/Asset/Unagi/Animations/SourceFBX'
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$projectFile = Join-Path $projectRoot 'ThirdPersonARPG.uproject'
if (-not $EngineRoot) {
    $association = (Get-Content -LiteralPath $projectFile -Raw -Encoding UTF8 | ConvertFrom-Json).EngineAssociation
    $install = Get-ItemProperty -LiteralPath "HKLM:/SOFTWARE/EpicGames/Unreal Engine/$association" -ErrorAction SilentlyContinue
    if ($install) { $EngineRoot = $install.InstalledDirectory }
    if (-not $EngineRoot) {
        $candidate = Join-Path ([IO.Path]::GetPathRoot($projectRoot)) "UE_$association"
        if (Test-Path -LiteralPath $candidate) { $EngineRoot = $candidate }
    }
}
if (-not $EngineRoot) { throw 'UE installation not found; supply -EngineRoot.' }
$editorCommand = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $editorCommand -PathType Leaf)) { throw "Not found: $editorCommand" }
if (-not $SourceDirectory) {
    # The resource folder has a localized name; select its numbered-FBX directory without encoding assumptions.
    $resource = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Content') -Directory -Filter '039-*')
    if ($resource.Count -ne 1) { throw 'Expected exactly one Content/039-* resource folder.' }
    $fbxRoot = Join-Path $resource[0].FullName 'fbx'
    $choices = @(Get-ChildItem -LiteralPath $fbxRoot -Directory | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName '1.fbx') })
    if ($choices.Count -ne 1) { throw 'Cannot uniquely locate numbered animation FBX files.' }
    $SourceDirectory = $choices[0].FullName
}
$SourceDirectory = (Resolve-Path -LiteralPath $SourceDirectory).Path
$workDirectory = Join-Path $projectRoot ('Saved/UnagiAnimationWork/' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $workDirectory -Force | Out-Null
$configFile = Join-Path $workDirectory 'config.json'
$reportFile = Join-Path $workDirectory 'import-report.json'
$config = @{
    mesh = $Mesh
    source_dir = $SourceDirectory
    destination = $Destination
    report_path = $reportFile
}
[IO.File]::WriteAllText($configFile, ($config | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
$previousConfig = $env:UNAGI_IMPORT_CONFIG
try {
    $env:UNAGI_IMPORT_CONFIG = $configFile
    $editorArgs = @(
        $projectFile, '-run=pythonscript',
        ('-script=' + (Join-Path $PSScriptRoot 'import_unagi_animations.py')),
        '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
        '-unattended', '-nop4', '-nosplash', '-NullRHI', '-nosound', '-NoLiveCoding',
        ('-abslog=' + (Join-Path $workDirectory 'import.log'))
    )
    & $editorCommand @editorArgs
    $editorExitCode = $LASTEXITCODE
    Write-Output "Import report: $reportFile"
    if ($editorExitCode -ne 0) { throw "UE import failed (exit $editorExitCode); inspect the log and report." }
    if (-not (Test-Path -LiteralPath $reportFile)) { throw 'UE did not write an import report.' }
    $report = Get-Content -LiteralPath $reportFile -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($report.status -ne 'complete') { throw "Import incomplete: $reportFile" }
    $report.clips | Select-Object asset,status,frames,seconds,bone_tracks | Format-Table -AutoSize
}
finally {
    $env:UNAGI_IMPORT_CONFIG = $previousConfig
}
