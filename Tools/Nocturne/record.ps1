# Offline recording: the game runs off screen on a fixed 1/60 s step and saves every frame (-dumpmovie), then ffmpeg
# turns the frames into an mp4 in Saved/Takes. Game time does not depend on how fast the machine renders, so a take is
# smooth and the same every time, and no window ever comes to the front. ffmpeg must be on PATH.
#   record.ps1 -Map /Game/UITests/NieR/Maps/NieR_TestMap -Seconds 3 -Name nier-test
#   record.ps1 -Map "/Engine/Maps/Entry?game=/Script/DreamGUIDemo.NocturneGameMode" -Seconds 10 -Name shot01 -Extra '-DemoShot=Test'
param(
    [Parameter(Mandatory)][string]$Map,
    [Parameter(Mandatory)][string]$Name,
    [double]$Seconds = 5,
    [int]$Fps = 60,
    [int]$Width = 1920,
    [int]$Height = 1080,
    [string]$Extra = '',
    [switch]$KeepFrames
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\Common.ps1')
$Shots   = Join-Path $DemoSaved 'Screenshots\WindowsEditor'
$Takes   = Join-Path $DemoSaved 'Takes'
$Frames  = Join-Path $Takes "$Name-frames"
$Log     = Join-Path $Takes "$Name.log"
$Mp4     = Join-Path $Takes "$Name.mp4"
New-Item -ItemType Directory -Force $Takes | Out-Null

# Frames from an earlier run would be mixed into this one.
if (Test-Path $Shots) { Get-ChildItem $Shots -Filter 'MovieFrame*' | Remove-Item -Force }
if (Test-Path $Frames) { Remove-Item -Recurse -Force $Frames }

# -ForceRes: without it the off-screen viewport ignores -ResX/-ResY and comes out at 1280x720 divided by the desktop's
# scale, and a UI designed for 1920x1080 is drawn too big.
$GameArgs = @($DemoProject, $Map, '-game', '-RenderOffScreen', "-ResX=$Width", "-ResY=$Height", '-ForceRes', '-windowed',
    '-benchmark', "-fps=$Fps", "-benchmarkseconds=$Seconds", '-dumpmovie', '-NoSound', '-unattended', '-nosplash',
    '-NoScreenMessages', "-abslog=$Log")
if ($Extra) { $GameArgs += ($Extra -split ' ') }

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
if (-not $KeepFrames) {
    # Keep a few stills for checking, drop the rest.
    $Keep = @(0, [int]($i / 4), [int]($i / 2), [int]($i * 3 / 4), ($i - 1)) | Select-Object -Unique
    Get-ChildItem $Frames | Where-Object { $Keep -notcontains [int]($_.BaseName.Substring(1)) } | Remove-Item -Force
}
