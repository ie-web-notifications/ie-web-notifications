@echo off
SETLOCAL
rem http://stackoverflow.com/questions/2872105/alternative-timestamping-services-for-authenticode

set SERVERLIST=(http://timestamp.verisign.com/scripts/timstamp.dll http://www.trustcenter.de/codesigning/timestamp http://timestamp.globalsign.com/scripts/timestamp.dll http://tsa.starfieldtech.com)

set timestampErrors=0

@for /L %%a in (1,1,3) do @(

    @for %%s in %SERVERLIST% do @(

        @rem try to timestamp the file. This operation is unreliable and may need to be repeated...
        @%~dp0signtool.exe timestamp /q /t %%s %1 2>nul 1>nul
        @echo with %%s

        if not ERRORLEVEL 1 GOTO succeeded

        @echo Signing failed. Probably cannot find the timestamp server at %%s
        set /a timestampErrors+=1
    )

    @rem wait 2 seconds...
    choice /N /T:2 /D:Y >NUL
)

echo Could not sign %1. There were %timestampErrors% timestamping errors.
exit /b 1

:succeeded
echo Successfully signed %1. There were %timestampErrors% timestamping errors.
exit /b 0
