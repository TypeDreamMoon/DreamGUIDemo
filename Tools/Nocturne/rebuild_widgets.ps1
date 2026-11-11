# Opens the demo editor, recompiles the named NOCTURNE//OS widget Blueprints from their .dui (one bridge exec each,
# a garbage collection between), prints the .dui diagnostics, and closes the editor again. Needs the optional
# UnrealBridge plugin and its skill in .claude/.
#   rebuild_widgets.ps1 -Names WBP_Nocturne_OptionRow,WBP_Nocturne_Settings
param([Parameter(Mandatory)][string[]]$Names)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\Common.ps1')
$Bridge = Join-Path $DemoRoot '.claude\skills\unreal-bridge\scripts\bridge.py'
$Make   = (Join-Path $PSScriptRoot 'make_nocturne_widgets.py') -replace '\\', '/'
$env:PYTHONIOENCODING = 'utf-8'

$Editor = Start-Process -FilePath $DemoEditor -ArgumentList "`"$DemoProject`"" -PassThru
$Deadline = (Get-Date).AddMinutes(7)
do {
    Start-Sleep -Seconds 10
    $Ping = python $Bridge --json --timeout 3 --project=DreamGUIDemo ping 2>$null
} until (($Ping -match '"ready": *true') -or (Get-Date) -gt $Deadline)
if (-not ($Ping -match '"ready": *true')) { throw 'editor did not come up' }

try {
    foreach ($Name in $Names) {
        python $Bridge --project=DreamGUIDemo --timeout 300 --no-preflight exec "NAME='$Name'; exec(open(r'$Make', encoding='utf-8').read())"
        python $Bridge --project=DreamGUIDemo --timeout 120 --no-preflight exec "import unreal; unreal.SystemLibrary.collect_garbage()" | Out-Null
    }
    $Filter = $Names | ForEach-Object { $_ -replace '^WBP_Nocturne_', '' }
    python (Join-Path $PSScriptRoot 'dui_diag.py') @Filter
}
finally {
    $null = $Editor.CloseMainWindow()
    if (-not $Editor.WaitForExit(180000)) { "editor still running after 180 s (pid $($Editor.Id))" } else { 'editor closed' }
}
