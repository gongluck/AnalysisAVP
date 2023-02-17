del /f /s /q "./build/android/*.*"
rd  /s /q "./build/android"

set buildprefix=./build/android
set buildparams=-DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" -DCMAKE_MAKE_PROGRAM=%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe -DANDROID_NDK=%ANDROID_NDK_HOME% -DANDROID_PLATFORM=android-22

rem armeabi-v7a
call:build armeabi-v7a

rem arm64-v8a
call:build arm64-v8a

rem x86
call:build x86

rem x86_64
call:build x86_64

goto EOF

rem params: abi(armeabi-v7a)
:build
cmake -G "Unix Makefiles" -S . -B %buildprefix%/%1_release %buildparams% -DANDROID_ABI=%1 -DCMAKE_BUILD_TYPE=release
cmake --build %buildprefix%/%1_release --clean-first --config release --target all -- -j8

:EOF