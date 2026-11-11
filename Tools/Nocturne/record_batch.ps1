# Records several NOCTURNE//OS shots one after another (they share the dumpmovie folder, so never in parallel) and
# prints each take's Nocturne log lines.
#   record_batch.ps1 -Shots Title:13,Archive:16.5
param([Parameter(Mandatory)][string[]]$Shots, [string]$Suffix = '01')

. (Join-Path $PSScriptRoot '..\Common.ps1')
$Takes = Join-Path $DemoSaved 'Takes'
foreach ($Shot in $Shots) {
    $Name, $Seconds = $Shot -split ':'
    $Take = "nocturne-$($Name.ToLower())$Suffix"
    "== $Take ($Seconds s)"
    & (Join-Path $PSScriptRoot 'record.ps1') -Map '/Engine/Maps/Entry?game=/Script/DreamGUIDemo.NocturneGameMode' -Seconds $Seconds -Name $Take -Extra "-DemoShot=$Name" -KeepFrames 2>&1 |
        Select-Object -Last 3
    Select-String -Path "$Takes\$Take.log" -Pattern 'LogNocturne|Error: |Ensure|Fatal' |
        Where-Object { $_.Line -notmatch 'LogAutomationTest|LogTemp' } |
        Select-Object -First 40 | ForEach-Object { $_.Line.Substring([Math]::Min(26, $_.Line.Length)) }
}
