del /f /s /q "./build/windows/*.*"
rd  /s /q "./build/windows"

rem win32
cmake -S . -B ./build/windows/win32 -G "Visual Studio 16 2019" -A win32
cmake --build ./build/windows/win32 --clean-first --config debug --target ALL_BUILD
cmake --build ./build/windows/win32 --clean-first --config release --target ALL_BUILD
rem x64
cmake -S . -B ./build/windows/x64 -G "Visual Studio 16 2019" -A x64
cmake --build ./build/windows/win32 --clean-first --config debug --target ALL_BUILD
cmake --build ./build/windows/x64 --clean-first --config release --target ALL_BUILD

pause