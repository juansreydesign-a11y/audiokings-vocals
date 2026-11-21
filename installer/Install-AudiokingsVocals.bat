@echo off
REM Audiokings Vocals - manual VST3 installer (run as Administrator).
REM Prefer the Setup .exe: it also removes old versions automatically.
set "DEST=%CommonProgramFiles%\VST3"
echo Removing previous versions ...
if exist "%DEST%\Audiokings Vocals.vst3" rmdir /s /q "%DEST%\Audiokings Vocals.vst3"
echo Installing Audiokings Vocals to "%DEST%" ...
if not exist "%DEST%" mkdir "%DEST%"
xcopy /E /I /Y "%~dp0Audiokings Vocals.vst3" "%DEST%\Audiokings Vocals.vst3"
if errorlevel 1 (
    echo.
    echo Installation failed. Right-click this file and choose "Run as administrator".
) else (
    echo.
    echo Done! Rescan plugins in FL Studio or Ableton Live.
)
pause
