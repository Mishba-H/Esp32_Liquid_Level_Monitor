@echo off
REM This batch file opens the two Arduino projects silently in the background.

REM The PowerShell command starts the process with its window hidden, 
REM which should prevent the black Arduino console window from appearing.
powershell -WindowStyle Hidden -Command "Start-Process -FilePath '.\Code_Arduino\Code_Arduino.ino'"

REM Wait for 5 seconds to allow the first IDE instance to open properly.
timeout /t 5 /nobreak >nul

REM Launch the second project silently.
powershell -WindowStyle Hidden -Command "Start-Process -FilePath '.\Code_Esp32\Code_Esp32.ino'"

exit

