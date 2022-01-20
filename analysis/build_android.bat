del /f /s /q "./build/android/*.*"
rd  /s /q "./build/android"

rem armeabi-v7a
cmake -G "Unix Makefiles" -S . -B ./build/android/armeabi-v7a -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" -DCMAKE_BUILD_TYPE=release -DCMAKE_MAKE_PROGRAM="%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe" -DANDROID_NDK="%ANDROID_NDK_HOME%" -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-22
cmake --build ./build/android/armeabi-v7a --clean-first --config release --target all

rem arm64-v8a
cmake -G "Unix Makefiles" -S . -B ./build/android/arm64-v8a -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" -DCMAKE_BUILD_TYPE=release -DCMAKE_MAKE_PROGRAM="%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe" -DANDROID_NDK="%ANDROID_NDK_HOME%" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-22
cmake --build ./build/android/arm64-v8a --clean-first --config release --target all

rem x86
cmake -G "Unix Makefiles" -S . -B ./build/android/x86 -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" -DCMAKE_BUILD_TYPE=release -DCMAKE_MAKE_PROGRAM="%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe" -DANDROID_NDK="%ANDROID_NDK_HOME%" -DANDROID_ABI=x86 -DANDROID_PLATFORM=android-22
cmake --build ./build/android/x86 --clean-first --config release --target all

rem x86_64
cmake -G "Unix Makefiles" -S . -B ./build/android/x86_64 -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake" -DCMAKE_BUILD_TYPE=release -DCMAKE_MAKE_PROGRAM="%ANDROID_NDK_HOME%/prebuilt/windows-x86_64/bin/make.exe" -DANDROID_NDK="%ANDROID_NDK_HOME%" -DANDROID_ABI=x86_64 -DANDROID_PLATFORM=android-22
cmake --build ./build/android/x86_64 --clean-first --config release --target all

pause