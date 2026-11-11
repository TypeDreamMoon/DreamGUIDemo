# Offline recording of the study: the game runs off screen on a fixed 1/Fps step and saves every frame (-dumpmovie),
# then ffmpeg (on PATH) makes an mp4 in Saved/Takes, and the frames at the -Stills times are kept as PNGs to look at.
# Game time does not depend on how fast the machine renders, so a take is the same every time, and no window comes to
# the front.
#   record.ps1 -Name tour -Seconds 24 -Keys '1.0:Tab,3.0:Down' -Stills '2.5,6,9.5'
param(
    [Parameter(Mandatory)][string]$Name,
    [double]$Seconds = 5,
    [int]$Fps = 30,
    [string]$Keys = '',
    [string]$Stills = '',
    [int]$Width = 1920,
    [int]$Height = 1080,
    [string]$Map = '/Game/P3R/Maps/P3R_Demo',
    [string]$Exec = ''
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\Common.ps1')
$Shots   = Join-Path $DemoSaved 'Screenshots\WindowsEditor'
$Takes   = Join-Path $DemoSaved 'Takes'
$Frames  = Join-Path $Takes "$Name-frames"
$Log     = Join-Path $Takes "$Name.log"
$Mp4     = Join-Path $Takes "$Name.mp4"
New-Item -ItemType Directory -Force $Takes | Out-Null

if (Test-Path $Shots) { Get-ChildItem $Shots -Filter 'MovieFrame*' | Remove-Item -Force }
if (Test-Path $Frames) { Remove-Item -Recurse -Force $Frames }

# -ForceRes: without it the off-screen viewport ignores -ResX/-ResY and comes out at 1280x720 over the desktop scale.
$GameArgs = @($DemoProject, $Map, '-game', '-RenderOffScreen', "-ResX=$Width", "-ResY=$Height", '-ForceRes', '-windowed',
    '-benchmark', "-fps=$Fps", "-benchmarkseconds=$Seconds", '-dumpmovie', '-NoSound', '-unattended', '-nosplash',
    '-NoScreenMessages', "-abslog=$Log")
if ($Keys) { $GameArgs += "-P3RKeys=$Keys" }
if ($Exec) { $GameArgs += "-ExecCmds=$Exec" }

$T0 = Get-Date
& $DemoEditorCmd @GameArgs | Out-Null
"game exit=$LASTEXITCODE  $([int]((Get-Date) - $T0).TotalSeconds) s"

$Got = @(Get-ChildItem $Shots -Filter 'MovieFrame*' -ErrorAction SilentlyContinue | Sort-Object Name)
"frames: $($Got.Count)"
if ($Got.Count -eq 0) { Select-String -Path $Log -Pattern 'Error|Fatal|ensure' | Select-Object -First 15 | ForEach-Object Line; exit 1 }

New-Item -ItemType Directory -Force $Frames | Out-Null
$Ext = $Got[0].Extension
$i = 0
foreach ($F in $Got) { Move-Item $F.FullName (Join-Path $Frames ('f{0:D5}{1}' -f $i, $Ext)); $i++ }

ffmpeg -hide_banner -loglevel error -y -framerate $Fps -i (Join-Path $Frames "f%05d$Ext") `
    -c:v libx264 -preset slow -crf 16 -pix_fmt yuv420p $Mp4
"mp4: $Mp4"

# Keep the stills asked for (as PNG, named by their time), drop every other frame.
$Keep = @()
foreach ($S in ($Stills -split ',' | Where-Object { $_ })) {
    $Index = [int]([double]$S * $Fps)
    $Src = Join-Path $Frames ('f{0:D5}{1}' -f $Index, $Ext)
    if (Test-Path $Src) {
        $Png = Join-Path $Takes ("{0}-{1}s.png" -f $Name, $S)
        ffmpeg -hide_banner -loglevel error -y -i $Src $Png
        $Keep += $Png
    }
}
Remove-Item -Recurse -Force $Frames
"stills:"; $Keep
Select-String -Path $Log -Pattern 'LogP3R|Error:|Ensure|DUI[0-9]{4}' | Select-Object -First 20 | ForEach-Object { $_.Line -replace '^\[[^\]]*\]\[[^\]]*\]', '' }
