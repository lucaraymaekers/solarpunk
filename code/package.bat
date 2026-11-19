@echo off
setlocal

cd %~dp0

:: Unpack arguments
for %%a in (%*) do set "%%~a=1"

set suffix=debug
set mode=
if "%release%"=="1" set suffix=release
if "%release%"=="1" set mode=-Release

set dir=muze_%suffix%

IF NOT EXIST ..\build mkdir ..\build
cd ..\build
call ..\code\build.bat -Muze -Slow %mode%

:: Create clean directory
IF EXIST %dir% rmdir /s /q %dir%
mkdir %dir%
cd %dir%

copy /Y ..\..\build\muze.exe
copy /Y ..\..\build\muze.pdb
copy /Y ..\..\build\muze_app.dll
copy /Y ..\..\build\muze_app.pdb
copy /Y ..\..\data\font_text_regular.ttf
copy /Y ..\..\data\font_icons.ttf
copy /Y ..\..\data\font_music_text.otf
copy /Y ..\..\data\muze\sounds\sounds.sf2
copy /Y ..\..\data\muze\song.muze_piece

cd ..
del %dir%.zip
7z a %dir%.zip %dir%\*
