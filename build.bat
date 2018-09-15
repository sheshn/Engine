@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 > NUL

set CONFIGURATION=debug
set BUILD_DIR=build\%CONFIGURATION%\
set INTERMEDIATES=%BUILD_DIR%intermediates\
for %%a in (.) do set CURRENT_FOLDER=%%~nxa

if not exist %INTERMEDIATES% mkdir %INTERMEDIATES%

pushd %BUILD_DIR%
cl /utf-8 /std:c++latest /DDEBUG /W3 /Zi /MDd /EHsc /Fointermediates\ /nologo ..\..\%CURRENT_FOLDER%\platform\win32\%CURRENT_FOLDER%.win32.cpp kernel32.lib user32.lib /link /OUT:%CURRENT_FOLDER%.exe /INCREMENTAL:NO /NOLOGO
popd
