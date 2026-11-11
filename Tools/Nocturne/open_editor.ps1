# Opens the demo editor and leaves it open, waiting until the UnrealBridge plugin answers (needs the optional
# UnrealBridge plugin and its skill in .claude/).
. (Join-Path $PSScriptRoot '..\Common.ps1')
$Bridge = Join-Path $DemoRoot '.claude\skills\unreal-bridge\scripts\bridge.py'

$Editor = Start-Process -FilePath $DemoEditor -ArgumentList "`"$DemoProject`"" -PassThru
$Deadline = (Get-Date).AddMinutes(7)
do {
    Start-Sleep -Seconds 10
    $Ping = python $Bridge --json --timeout 3 --project=DreamGUIDemo ping 2>$null
} until (($Ping -match '"ready": *true') -or (Get-Date) -gt $Deadline)
"editor pid $($Editor.Id) ready=$($Ping -match '"ready": *true')"
