@echo off

setlocal

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 > NUL

set CONFIGURATION=debug
set BUILD_DIR=build\%CONFIGURATION%\
set INTERMEDIATES=%BUILD_DIR%intermediates\
set SHADERS=%BUILD_DIR%shaders\
for %%a in (.) do set CURRENT_FOLDER=%%~nxa

if not exist %INTERMEDIATES% mkdir %INTERMEDIATES%
if not exist %SHADERS% mkdir %SHADERS%

rem TODO: Improve shader build system
pushd %CURRENT_FOLDER%
call %VK_SDK_PATH%\Bin\glslangValidator.exe -t -V -o ..\%SHADERS%\vertex.spv platform\vulkan\shaders\shader.vert
call %VK_SDK_PATH%\Bin\glslangValidator.exe -t -V -o ..\%SHADERS%\fragment.spv platform\vulkan\shaders\shader.frag
popd

pushd %BUILD_DIR%
cl /utf-8 /std:c++latest /DDEBUG /W3 /Zi /MDd /EHsc /Fointermediates\ /nologo ..\..\%CURRENT_FOLDER%\platform\win32\%CURRENT_FOLDER%.win32.cpp kernel32.lib user32.lib /link /OUT:%CURRENT_FOLDER%.exe /INCREMENTAL:NO /NOLOGO
popd

endlocal
