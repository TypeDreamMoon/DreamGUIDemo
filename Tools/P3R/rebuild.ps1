# One pass of the whole P3R study pipeline after a round of edits: art, C++, textures, widgets, paths scrubbed, then the
# tour take and the reference comparison (the comparison needs the reference shots in Plan/P3R/ref, which are not
# committed).
param([string]$Take = 'p3r-study-tour', [switch]$Shaders, [switch]$NoTake)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\Common.ps1')
Set-Location $DemoRoot
$Commandlet = Join-Path $PSScriptRoot '..\run-commandlet.ps1'

python Art/P3R/prepare_art.py | Select-Object -Last 3
& $DemoBuildBat DreamGUIDemoEditor Win64 Development "-Project=$DemoProject" -WaitMutex -NoHotReloadFromIDE 2>&1 | Select-String -Pattern 'error|Result:' | Select-Object -Last 10
if ($LASTEXITCODE -ne 0) { 'build failed'; exit 1 }
& $Commandlet -Run P3RSetup -Args '-Textures' | Select-Object -Last 2
if ($Shaders) { & (Join-Path $PSScriptRoot 'shaders.ps1') }
& $Commandlet -Run DreamUICompileWidgets -Args '-Manifest=Tools/P3R/widgets.json' | Select-String 'compiled,|failed' | Select-Object -Last 3
& $Commandlet -Run ScrubImportPaths -Args '-Path=/Game/P3R' | Select-Object -Last 1
if ($NoTake) { return }
& (Join-Path $PSScriptRoot 'record.ps1') -Name $Take -Seconds 26 -Fps 60 -Keys '1.0:Tab,2.6:Move@968;404,3.1:Move@950;530,3.6:Move@984;634,4.3:Enter,5.6:Down,6.2:Down,6.8:Down,7.4:E,8.2:Escape,9.2:Down,9.5:Down,10.1:Enter,11.3:Down,11.9:Down,12.8:Escape,13.8:Down,14.1:Down,14.7:Enter,15.8:Down,16.2:Down,17.2:Escape,18.4:Tab,20.6:Tab,22.8:Tab,24.4:Escape' -Stills '4.0,7.2,12.5,17.0,20.3,22.5,3.15,4.45,5.65,8.27,11.4' 2>&1 | Select-Object -Last 3
python Tools/P3R/pairs.py $Take
python Tools/P3R/compare.py $Take
