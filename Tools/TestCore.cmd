@echo off
setlocal
call "C:\BuildTools2022\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
pushd "%~dp0.."
if not exist artifacts mkdir artifacts
cl /nologo /std:c++17 /EHsc /W4 /O2 Tests\DiveModelTests.cpp /Fo:artifacts\DiveModelTests.obj /Fe:artifacts\DiveModelTests.exe
if errorlevel 1 exit /b 1
artifacts\DiveModelTests.exe
set result=%errorlevel%
popd
exit /b %result%
