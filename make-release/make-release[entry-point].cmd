@echo off
SETLOCAL
IF [%CERTPATH%] == [] goto CERTPATH_is_not_set

@call %~dp0compile-release.cmd

@pushd %~dp0..\build

@set FILELIST=(Win32_Release\ie_web_notifications.dll Win32_Release\ie_web_notifications_server.exe x64_Release\ie_web_notifications.dll x64_Release\ie_web_notifications_server.exe Win32_Release\ie_web_notifications.toast.dll x64_Release\ie_web_notifications.toast.dll Win32_Release\ie_web_notifications.d2d.dll x64_Release\ie_web_notifications.d2d.dll)

@for %%f in %FILELIST% do @(
  @%~dp0signtool.exe sign /fd "SHA256" /d "IE Web Notifications" /du "https://ie-web-notifications.github.io/" /q /f %CERTPATH% %%f
  @call %~dp0signtool-add-timestamp.cmd %%f
)

@popd

@call %~dp0compile-msi.cmd

@pushd %~dp0..\ie_web_notifications.setup\bin
@set MSIFILELIST=(Release\ie_web_notifications.setup.msi x64\Release\ie_web_notifications.setup.msi)

@for %%f in %MSIFILELIST% do @(
  @%~dp0signtool.exe sign /fd "SHA256" /d "IE Web Notifications setup" /du "https://ie-web-notifications.github.io/" /q /f %CERTPATH% %%f
  @call %~dp0signtool-add-timestamp.cmd %%f
)

@popd

goto succeeded

:CERTPATH_is_not_set
@echo set CERTPATH, e.g. set CERTPATH="F:\!private\codesign.pfx"
:succeeded
ENDLOCAL
exit /b 0
