@echo off
setlocal

:: ----------------------------------------------------------

cd %~dp0

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

IF NOT EXIST .\cling.exe (
  cl -MTd -Gm- -nologo -GR- -EHa- -Oi -FC -Zi -WX -W4 -wd4459 -wd4456 -wd4201 -wd4100 -wd4101 -wd4189 -wd4063 -wd4505 -wd4996 -wd4389 -wd4244 -wd4702 -wd5287 -I..\code -Zc:strictStrings- -I..\code -Fecling.exe -Fmcling.map ..\code\cling\cling.c /link -opt:ref -incremental:no
)

IF NOT EXIST font_text_regular.ttf copy /Y ..\data\font_text_regular.ttf
IF NOT EXIST font_icons.ttf        copy /Y ..\data\font_icons.ttf

SET CLING_NOREBUILD=1
.\cling.exe %*

:: ----------------------------------------------------------