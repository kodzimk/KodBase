@echo off
rem launch this from msvc-enabled console


set CFLAGS=/std:c11 /O2 /FC /W4 /WX /wd4996 /nologo
set LIBS=

cl.exe %CFLAGS% ./src/kodi.c

if "%1" == "examples" setlocal EnableDelayedExpansion && for /F "tokens=*" %%e in ('dir /b .\examples\*.kb') do (
    set name=%%e
    "./kodi.exe" .\examples\%%e 
)