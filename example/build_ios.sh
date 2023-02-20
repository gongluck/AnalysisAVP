rm -r ./build/ios

cmake -S . -B ./build/ios -G Xcode -DENABLE_BITCODE=NO \
  -DCMAKE_TOOLCHAIN_FILE=../3rdparty/ios-cmake/ios.toolchain.cmake -DPLATFORM=OS64 \
  -DBUILD_SHARED_LIBS=OFF
cmake --build ./build/ios --clean-first --target ALL_BUILD --config Release
