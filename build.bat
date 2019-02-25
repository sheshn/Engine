@echo off

setlocal

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 > NUL

set BUILD_DIR=build\
set INTERMEDIATES=%BUILD_DIR%intermediates\
set SHADERS=%BUILD_DIR%shaders\
for %%a in (.) do set CURRENT_FOLDER=%%~nxa

if not exist %INTERMEDIATES% mkdir %INTERMEDIATES%
if not exist %SHADERS% mkdir %SHADERS%

:: TODO: Improve shader build system
pushd %CURRENT_FOLDER%
call %VK_SDK_PATH%\Bin\glslangValidator.exe -t -V -o ..\%SHADERS%\vertex.spv platform\vulkan\shaders\shader.vert
call %VK_SDK_PATH%\Bin\glslangValidator.exe -t -V -o ..\%SHADERS%\fragment.spv platform\vulkan\shaders\shader.frag
popd

pushd %BUILD_DIR%
:: Asset Builder
:: cl /utf-8 /std:c++latest /O2 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /W3 /Zi /MDd /EHsc /Fointermediates\ /nologo ..\tools\%CURRENT_FOLDER%.asset.builder.cpp kernel32.lib user32.lib /link /OUT:%CURRENT_FOLDER%.asset.builder.exe /INCREMENTAL:NO /NOLOGO

:: Vulkan Generator
:: cl /utf-8 /std:c++latest /O2 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /W3 /Zi /MDd /EHsc /Fointermediates\ /nologo ..\tools\%CURRENT_FOLDER%.vulkan.generator.cpp kernel32.lib user32.lib /link /OUT:%CURRENT_FOLDER%.vulkan.generator.exe /INCREMENTAL:NO /NOLOGO
call %CURRENT_FOLDER%.vulkan.generator.exe %VK_SDK_PATH%\Include\vulkan\vulkan_core.h ..\%CURRENT_FOLDER%\platform\vulkan\%CURRENT_FOLDER%.vulkan.cpp ..\%CURRENT_FOLDER%\platform\vulkan\%CURRENT_FOLDER%.vulkan.generated.h

:: Engine
cl /utf-8 /DDEBUG /Od /Oi /W3 /Zi /EHa- /GR- /Gm- /GS- /Gs9999999 /Fointermediates\ /nologo ..\%CURRENT_FOLDER%\platform\win32\%CURRENT_FOLDER%.win32.cpp ..\%CURRENT_FOLDER%\platform\win32\%CURRENT_FOLDER%.win32.msvc.cpp kernel32.lib user32.lib /link /OUT:%CURRENT_FOLDER%.exe /INCREMENTAL:NO /NOLOGO /NODEFAULTLIB /SUBSYSTEM:windows /STACK:0x100000,0x100000
popd

endlocal