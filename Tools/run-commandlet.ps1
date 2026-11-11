# Runs one editor commandlet on the project headlessly and prints what matters from its log (Saved/Commandlets/<name>.log).
#   run-commandlet.ps1 -Run P3RSetup
#   run-commandlet.ps1 -Run DreamUICompileWidgets -Args '-Manifest=Tools/P3R/widgets.json'
#   run-commandlet.ps1 -Run ScrubImportPaths                 (before committing imported or regenerated assets)
param(
    [Parameter(Mandatory)][string]$Run,
    [string]$Args = '',
    [string]$Pattern = 'Error|Warning: .*(P3R|DreamUI|Dream)|P3RSetup|ScrubImportPaths|DreamUICompileWidgets|compiled and saved|FAILED| saved'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$Logs = Join-Path $DemoSaved 'Commandlets'
New-Item -ItemType Directory -Force $Logs | Out-Null
$Log = Join-Path $Logs "$Run.log"

$CmdArgs = @($DemoProject, "-run=$Run", '-unattended', '-nopause', '-nosplash', "-abslog=$Log") + ($Args -split ' ' | Where-Object { $_ })
$T0 = Get-Date
Push-Location $DemoRoot
try { & $DemoEditorCmd @CmdArgs | Out-Null; $Exit = $LASTEXITCODE } finally { Pop-Location }
"$Run exit=$Exit  $([int]((Get-Date) - $T0).TotalSeconds) s"
Select-String -Path $Log -Pattern $Pattern | Select-Object -Last 80 | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
exit $Exit
