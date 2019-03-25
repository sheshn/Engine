@echo off

setlocal

set BUILD_DIR=build\
set INTERMEDIATES=%BUILD_DIR%intermediates\
for %%a in (.) do set CURRENT_FOLDER=%%~nxa

if not exist %INTERMEDIATES% mkdir %INTERMEDIATES%

pushd %BUILD_DIR%
:: Asset Builder
:: clang ..\tools\%CURRENT_FOLDER%.asset.builder.cpp -o%CURRENT_FOLDER%.asset.builder.exe -D_CRT_SECURE_NO_WARNINGS -DUNICODE -DDEBUG -O0 -g -gcodeview -gno-column-info -fno-cxx-exceptions -Wno-writable-strings -Wno-switch --for-linker -machine:x64 --for-linker -incremental:no --for-linker -opt:ref --for-linker -subsystem:console

:: Vulkan Generator
:: clang ..\tools\%CURRENT_FOLDER%.vulkan.generator.cpp -o%CURRENT_FOLDER%.vulkan.generator.exe -D_CRT_SECURE_NO_WARNINGS -DUNICODE -O3 -g -gcodeview -gno-column-info -fno-cxx-exceptions -Wno-writable-strings -Wno-switch --for-linker -machine:x64 --for-linker -incremental:no --for-linker -opt:ref --for-linker -subsystem:console
call %CURRENT_FOLDER%.vulkan.generator.exe %VK_SDK_PATH% ..\%CURRENT_FOLDER%\platform\vulkan\%CURRENT_FOLDER%.vulkan.cpp ..\%CURRENT_FOLDER%\platform\vulkan\shaders ..\%CURRENT_FOLDER%\platform\vulkan\%CURRENT_FOLDER%.vulkan.generated.h

:: Job Test
:: clang ..\tools\%CURRENT_FOLDER%.job.test.cpp -o%CURRENT_FOLDER%.job.test.exe -O3 -g -gcodeview -gno-column-info -fno-exceptions -fno-cxx-exceptions -fno-rtti -mno-stack-arg-probe -Wno-writable-strings --for-linker -machine:x64 --for-linker -incremental:no --for-linker -opt:ref --for-linker -subsystem:console --for-linker -stack:0x100000,0x100000

:: Engine
clang ..\%CURRENT_FOLDER%\platform\win32\%CURRENT_FOLDER%.win32.cpp -o%CURRENT_FOLDER%.exe -DDEBUG -O0 -g -gcodeview -gno-column-info -fno-exceptions -fno-cxx-exceptions -fno-rtti -mno-stack-arg-probe -Wno-writable-strings --for-linker -machine:x64 --for-linker -nodefaultlib --for-linker -incremental:no --for-linker -opt:ref --for-linker -subsystem:windows --for-linker -stack:0x100000,0x100000 -luser32.lib -lkernel32.lib
popd

endlocal