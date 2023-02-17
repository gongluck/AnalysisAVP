del /f /s /q "./build/windows/*.*"
rd  /s /q "./build/windows"

rem win32
call:build win32
rem x64
call:build x64

goto EOF

rem params: platform(win32)
:build
cmake -S . -B ./build/windows/%1 -G "Visual Studio 16 2019" -A %1 ^
-DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:>"
cmake --build ./build/windows/%1 --clean-first --config release --target ALL_BUILD

:EOF
