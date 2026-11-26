@echo off
setlocal
call "C:\BuildTools2022\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts mkdir artifacts
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\DiveModelTests.cpp /Fo:artifacts\DiveModelTests.obj /Fe:artifacts\DiveModelTests.exe
if errorlevel 1 exit /b 1
artifacts\DiveModelTests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\CoastGridTests.cpp /Fo:artifacts\CoastGridTests.obj /Fe:artifacts\CoastGridTests.exe
if errorlevel 1 exit /b 1
artifacts\CoastGridTests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\DecompressionTests.cpp /Fo:artifacts\DecompressionTests.obj /Fe:artifacts\DecompressionTests.exe
if errorlevel 1 exit /b 1
artifacts\DecompressionTests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\FishMotionTests.cpp /Fo:artifacts\FishMotionTests.obj /Fe:artifacts\FishMotionTests.exe
if errorlevel 1 exit /b 1
artifacts\FishMotionTests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\FishSteeringTests.cpp /Fo:artifacts\FishSteeringTests.obj /Fe:artifacts\FishSteeringTests.exe
if errorlevel 1 exit /b 1
artifacts\FishSteeringTests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\BedDistributionTests.cpp /Fo:artifacts\BedDistributionTests.obj /Fe:artifacts\BedDistributionTests.exe
if errorlevel 1 exit /b 1
artifacts\BedDistributionTests.exe
set result=%errorlevel%
popd
exit /b %result%
