# Compiles the study's DreamShader sources headlessly (all of them, or the names given).
#   shaders.ps1                 shaders.ps1 -Names M_P3R_Sea,M_P3R_InkGlass
param([string[]]$Names = @('M_P3R_Sea', 'M_P3R_InkGlass', 'M_P3R_BubbleSprite', 'M_P3R_ShardSprite', 'M_P3R_Figure'))
. (Join-Path $PSScriptRoot '..\Common.ps1')
Set-Location $DemoRoot
$Failed = 0
foreach ($M in $Names) {
    $Out = pwsh -NoProfile -File Plugins/DreamShader/.skill/dsc.ps1 compile "DShader/P3R/Materials/$M.dss" -Force 2>&1
    if ($LASTEXITCODE -ne 0) { $Failed++ }
    "$M : " + (($Out | Select-String 'dsc: (OK|FAILED)' | Select-Object -Last 1).Line)
    $Out | Select-String 'Error|DSH[0-9]{4}' | Select-Object -First 8 | ForEach-Object { '    ' + $_.Line }
}
exit $Failed
