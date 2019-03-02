@echo off

setlocal

set BUILD_DIR=build\
set INTERMEDIATES=%BUILD_DIR%intermediates\
set SHADERS=%BUILD_DIR%shaders\
for %%a in (.) do set CURRENT_FOLDER=%%~nxa

if not exist %INTERMEDIATES% mkdir %INTERMEDIATES%
if not exist %SHADERS% mkdir %SHADERS%

set WINDOWS_SDK_VERSION=10.0.17763.0
set WINDOWS_SDK_PATH=C:\Program Files (x86)\Windows Kits\10

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
clang ..\%CURRENT_FOLDER%\platform\win32\%CURRENT_FOLDER%.win32.cpp -o%CURRENT_FOLDER%.exe -DDEBUG -O0 -g -gcodeview -gno-column-info -fno-exceptions -fno-cxx-exceptions -fno-rtti -mno-stack-arg-probe -Wno-writable-strings --for-linker -machine:x64 --for-linker -nodefaultlib --for-linker -incremental:no --for-linker -opt:ref --for-linker -subsystem:windows --for-linker -stack:0x100000,0x100000 --for-linker -libpath:"%WINDOWS_SDK_PATH%\Lib\%WINDOWS_SDK_VERSION%\um\x64" -luser32.lib -lkernel32.lib
popd

endlocal