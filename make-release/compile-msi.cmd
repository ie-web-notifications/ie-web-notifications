@echo off
SETLOCAL

@set myVCDir="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC"
@pushd %myVCDir%
@call %myVCDir%\vcvarsall.bat x64
@popd

@pushd %~dp0..
@msbuild ie_web_notifications.setup.sln /p:Configuration=Release /p:Platform=x86
@msbuild ie_web_notifications.setup.sln /p:Configuration=Release /p:Platform=x64
@popd

ENDLOCAL
exit /b 0
