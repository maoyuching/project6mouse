@echo off
echo Compiling Mouse Enhancer...
gcc main.c -o mouse_enhancer.exe -mwindows -luser32 -lgdi32 -lcomctl32 -lgdiplus
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo Run mouse_enhancer.exe to start the application.
) else (
    echo Compilation failed!
)
pause
