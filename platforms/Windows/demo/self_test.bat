@echo off
setlocal
cd /d "%~dp0"
if not exist "bin\hiface_cli.exe" (
  echo RESULT: FAIL - bin\hiface_cli.exe is missing.
  pause
  exit /b 1
)
"%~dp0bin\hiface_cli.exe" self-test
set "HiFaceResult=%ERRORLEVEL%"
echo.
if "%HiFaceResult%"=="0" (
  echo RESULT: PASS
) else (
  echo RESULT: FAIL ^(exit code %HiFaceResult%^)
)
echo Logs: %%LOCALAPPDATA%%\HiFaceDemo\logs
pause
exit /b %HiFaceResult%
