# Headless load check for the demo project.
#   1. CompileAllBlueprints over /Game (every widget Blueprint of every study included).
#   2. Each demo map run as a game for a few seconds, off screen (real RHI, no window).
# Logs go to Saved\Checks\<tag>-*.log; a short summary of every warning/error that matters is printed.
param(
    [string]$Tag = 'load1',
    [int]$Seconds = 25,
    [switch]$SkipBlueprints,
    [switch]$SkipMaps
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$Out = Join-Path $DemoSaved 'Checks'
New-Item -ItemType Directory -Force $Out | Out-Null

function Show-Problems([string]$Log) {
    if (-not (Test-Path $Log)) { "  (no log)"; return }
    $Lines = Select-String -Path $Log -Pattern 'Error:|Warning:|Failed to load|Missing|Unable to load|NieR UI|Detroit UI|ensure|Fatal' |
        Where-Object { $_.Line -notmatch 'LogStreaming: Warning: .*(Doesn.t exist|while loading).*/Engine/' } |
        ForEach-Object { $_.Line -replace '^\[[^\]]+\]\[\s*\d+\]', '' } |
        Group-Object | Sort-Object Count -Descending | Select-Object -First 40
    if (-not $Lines) { "  clean"; return }
    foreach ($L in $Lines) { '  {0,3}x {1}' -f $L.Count, $L.Name.Trim().Substring(0, [Math]::Min(220, $L.Name.Trim().Length)) }
}

if (-not $SkipBlueprints) {
    $Log = Join-Path $Out "$Tag-blueprints.log"
    $T0 = Get-Date
    & $DemoEditorCmd $DemoProject -run=CompileAllBlueprints -unattended -nullrhi -nosplash -NoLogTimes "-abslog=$Log" | Out-Null
    "== CompileAllBlueprints exit=$LASTEXITCODE  $([int]((Get-Date) - $T0).TotalSeconds) s"
    Select-String -Path $Log -Pattern 'Compiling Completed|Blueprints? (that )?(failed|with errors)|Total' | Select-Object -Last 6 | ForEach-Object { '  ' + $_.Line }
    Show-Problems $Log
}

if (-not $SkipMaps) {
    foreach ($Map in '/Game/Nocturne/Maps/Nocturne_Demo', '/Game/P3R/Maps/P3R_Demo', '/Game/UITests/NieR/Maps/NieR_TestMap', '/Game/UITests/Detroit/Maps/Detroit_TestMap') {
        $Short = ($Map -split '/')[-1]
        $Log = Join-Path $Out "$Tag-$Short.log"
        $T0 = Get-Date
        & $DemoEditorCmd $DemoProject $Map -game -RenderOffScreen -unattended -nosplash -NoSound -ResX=1920 -ResY=1080 -ForceRes -windowed `
            -benchmark "-benchmarkseconds=$Seconds" "-abslog=$Log" | Out-Null
        "== $Short exit=$LASTEXITCODE  $([int]((Get-Date) - $T0).TotalSeconds) s"
        Show-Problems $Log
    }
}
