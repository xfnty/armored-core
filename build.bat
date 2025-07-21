@echo off
setlocal enableextensions enabledelayedexpansion

for %%a in (%*) do set "%%a=1"
if "%dist%"=="" set "debug=1"
if "%debug%"=="1" if "%dist%"=="1" (echo Can not build both "debug" and "dist" at once. && exit /b 1)
if "%debug%"=="1" set "config=debug"
if "%dist%"=="1" set "config=dist"

set "deps_dir=%~dp0tmp\deps\"
set "tmp=%~dp0tmp\%config%\"
set "out=%~dp0out\%config%\"
set "src=%~dp0src"

call :SetupDeps || (echo Failed to setup dependencies. && exit /b 1)
call :Build || (echo Build failed. && exit /b 1)
if "%run%"=="1" call :Run || exit /b 1
exit /b 0

:SetupDeps
if exist "%deps_dir%sdl" exit /b 0
echo Downloading SDL3 ...
mkdir "%deps_dir%sdl" 2> nul
bitsadmin /transfer DownloadSDL /download "https://github.com/libsdl-org/SDL/releases/download/release-3.2.18/SDL3-devel-3.2.18-VC.zip" "%deps_dir%sdl\SDL3-devel-3.2.18-VC.zip" 1> nul || (echo failed. && exit /b 1)
pushd "%deps_dir%sdl" && tar -xf SDL3-devel-3.2.18-VC.zip && popd
del /f /s /q "%deps_dir%sdl\SDL3-devel-3.2.18-VC.zip" 1> nul
exit /b 0

:Build
mkdir "%tmp%" "%out%" 2> nul
echo.> "%tmp%unity.c"
for /r %src% %%c in (*.c) do echo #include "%%c">> "%tmp%unity.c"
if "%config%"=="debug" set "cflags=/Z7 /Ob2"
if "%config%"=="debug" set "lflags=/debug"
cl.exe /nologo /options:strict /std:c11 /Oi- /GS- /fp:fast !cflags! /I %src%. /I %deps_dir%sdl\SDL3-3.2.18\include %tmp%unity.c %deps_dir%sdl\SDL3-3.2.18\lib\x64\SDL3.lib opengl32.lib /Fe:%out%emu.exe /Fo:%tmp%unity.obj /link /subsystem:console !lflags! || exit /b 1
xcopy /y %deps_dir%sdl\SDL3-3.2.18\lib\x64\SDL3.dll %out% 1> nul
exit /b 0

:Run
%out%emu.exe
if not !ERRORLEVEL!==0 (
    echo Failed with code !ERRORLEVEL!.
    exit /b 1
)
exit /b 0
