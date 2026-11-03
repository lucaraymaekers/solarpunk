@echo off
setlocal

cd %~dp0

IF NOT EXIST ..\build mkdir ..\build
cd ..\build
call ..\code\build.bat -Muze -Slow

REM --- Delete muze directory
IF EXIST .\muze rmdir /s /q muze
mkdir .\muze 
cd .\muze

copy /Y ..\..\build\muze.exe
copy /Y ..\..\build\muze_app.dll
copy /Y ..\..\data\font_regular.ttf
copy /Y ..\..\data\icons.ttf
copy /Y ..\..\data\muze\sounds\sounds.sf2
copy /Y ..\..\data\muze\song.muze_piece

del *.zip
7z a muze.zip *