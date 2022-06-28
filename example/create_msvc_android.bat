rd /q /s "./build/android/armeabi-v7a_msvc"

echo "use ndk: %ANDROID_NDK_HOME%"
cmake -G "Visual Studio 16 2019" -A ARM -S . -B ./build/android/armeabi-v7a_msvc ^
-DANDROID_ABI=armeabi-v7a ^
-DANDROID_PLATFORM=android-22 ^
-DCMAKE_BUILD_TYPE=release ^
-DANDROID_NDK="%ANDROID_NDK_HOME%" ^
-DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake"
pause