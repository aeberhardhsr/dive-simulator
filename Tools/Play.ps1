param([string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8')
$project = Join-Path (Split-Path $PSScriptRoot -Parent) 'divingsim/divingsim.uproject'
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" $project /Game/Diving/Maps/L_DiveTraining -game -windowed -ResX=1600 -ResY=900 -NoSplash '-log=DivePlay.log'
