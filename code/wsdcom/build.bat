@echo off

setlocal
pushd Debug

set SOURCE=../Download.c
set COMPFLAGS=-MTd -Gm- -nologo -GR- -EHa- -Oi -FC -Z7
set LINKfLAGS=/link -opt:ref -incremental:no
cl %COMPFLAGS% %SOURCE% %LINKFLAGS%  
