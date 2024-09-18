export ANDROID_NDK_ROOT=/root/android-ndk-r20b
export ANDROID_PLATFORM_DIR=/root/android10/platform
export PREBUILT=arm64

export platform=android-29
export abi=arm64-v8a

export BUILDPATH=$(pwd)/build
export INSTALLPATH=$(pwd)/bin

rm -r $BUILDPATH
rm -r $INSTALLPATH

aidl -h build/aidl/include -o build/aidl/cpp --lang=cpp $ANDROID_PLATFORM_DIR/frameworks/av/media/libmedia/aidl/android/IOMXNode.aidl
aidl -I $ANDROID_PLATFORM_DIR/frameworks/av/media/libmedia/aidl -h build/aidl/include -o build/aidl/cpp --lang=cpp $ANDROID_PLATFORM_DIR/frameworks/av/media/libmedia/aidl/android/IGraphicBufferSource.aidl

export buildparams="-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake -DCMAKE_MAKE_PROGRAM=$ANDROID_NDK_ROOT/prebuilt/linux-x86_64/bin/make"

cmake -G "Unix Makefiles" -S . -B $BUILDPATH \
   -DANDROID_NDK=$ANDROID_NDK_ROOT $buildparams \
   -DCMAKE_BUILD_TYPE=release -DANDROID_ABI=$abi -DANDROID_PLATFORM=$platform \
   -DANDROID_PLATFORM_DIR=$ANDROID_PLATFORM_DIR -DPREBUILT=$PREBUILT
cmake --build $BUILDPATH --clean-first --config release --target all -- -j1
