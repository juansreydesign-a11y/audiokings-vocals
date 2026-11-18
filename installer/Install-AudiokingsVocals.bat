@echo off
REM Audiokings Vocals - VST3 installer (run as Administrator)
set "DEST=%CommonProgramFiles%\VST3"
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
