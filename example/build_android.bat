del /f /s /q "./build/android/*.*"
rd  /s /q "./build/android"

set buildprefix=./build/android
set buildparams=-DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" -DCMAKE_MAKE_PROGRAM=%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe -DANDROID_NDK=%ANDROID_NDK_HOME% -DANDROID_PLATFORM=android-22

rem armeabi-v7a
set abi=armeabi-v7a
set type=debug
set buildpath=%buildprefix%/%abi%_%type%
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8
set type=release
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8

rem arm64-v8a
set abi=arm64-v8a
set type=debug
set buildpath=%buildprefix%/%abi%_%type%
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8
set type=release
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8

rem x86
set abi=x86
set type=debug
set buildpath=%buildprefix%/%abi%_%type%
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8
set type=release
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8

rem x86_64
set abi=x86_64
set type=debug
set buildpath=%buildprefix%/%abi%_%type%
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8
set type=release
cmake -G "Unix Makefiles" -S . -B %buildpath% %buildparams% -DANDROID_ABI=%abi% -DCMAKE_BUILD_TYPE=%type%
cmake --build %buildpath% --clean-first --config %type% --target all -- -j8

pause