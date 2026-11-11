# Dot-sourced by the PowerShell tools: where the project and the engine are.
#
# The engine is $env:UE_ENGINE_ROOT when set, else the UE 5.8 the Epic launcher installed, else a source build
# registered as "5.8". Tools/Local.ps1 (not committed) is read first and may set UE_ENGINE_ROOT for this machine.
# DreamShader's dsc.ps1 and DreamFX's dfx.ps1 read UE_ENGINE_ROOT as well, so it is exported.

$LocalConfig = Join-Path $PSScriptRoot 'Local.ps1'
if (Test-Path $LocalConfig) { . $LocalConfig }

$DemoRoot    = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$DemoProject = Join-Path $DemoRoot 'DreamGUIDemo.uproject'
$DemoSaved   = Join-Path $DemoRoot 'Saved'

function Find-DemoEngine {
    if ($env:UE_ENGINE_ROOT -and (Test-Path (Join-Path $env:UE_ENGINE_ROOT 'Engine\Binaries\Win64'))) { return $env:UE_ENGINE_ROOT }
    $Launcher = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path $Launcher) {
        $Install = (Get-Content $Launcher -Raw | ConvertFrom-Json).InstallationList | Where-Object AppName -eq 'UE_5.8' | Select-Object -First 1
        if ($Install) { return $Install.InstallLocation }
    }
    $Builds = Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds' -ErrorAction SilentlyContinue
    if ($Builds -and $Builds.'5.8') { return $Builds.'5.8' }
    throw "Unreal Engine 5.8 not found: set UE_ENGINE_ROOT to its folder (the one holding Engine\), or set it in $LocalConfig."
}

$DemoEngine    = Find-DemoEngine
$env:UE_ENGINE_ROOT = $DemoEngine
$DemoEditor    = Join-Path $DemoEngine 'Engine\Binaries\Win64\UnrealEditor.exe'
$DemoEditorCmd = Join-Path $DemoEngine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$DemoBuildBat  = Join-Path $DemoEngine 'Engine\Build\BatchFiles\Build.bat'

# Derived-data locations set as user variables reach this process only if it was started after they were set.
foreach ($Var in 'UE-ZenDataPath', 'UE-LocalDataCachePath') {
    $Value = [Environment]::GetEnvironmentVariable($Var, 'User')
    if ($Value) { [Environment]::SetEnvironmentVariable($Var, $Value, 'Process') }
}
