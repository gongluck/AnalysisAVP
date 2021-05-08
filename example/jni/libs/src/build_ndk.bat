set runpath=%cd%

set buildpath="./build/armeabi-v7a"
set abi="armeabi-v7a"

rmdir /s /q %buildpath%

cmake -G "Unix Makefiles" -S ./ -B %buildpath% ^
-DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" ^
-DANDROID_NDK="%ANDROID_NDK_HOME%" ^
-DCMAKE_BUILD_TYPE=Debug ^
-DANDROID_ABI=%abi% ^
-DCMAKE_MAKE_PROGRAM="%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe" ..
cmake --build %buildpath% --clean-first --config Debug --target all

pause