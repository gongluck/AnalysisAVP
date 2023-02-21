rm -r ./build/apple/*

# params: system(iOS) arch("armv7;arm64;i386;x86_64")
function build {
  cmake -S . -B ./build/apple/$1 -GXcode \
    -DCMAKE_SYSTEM_NAME=$1 \
    -DCMAKE_OSX_ARCHITECTURES=$2 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=9.0 \
    -DCMAKE_INSTALL_PREFIX=$(pwd)/bin/apple/$1 \
    -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
    -DCMAKE_IOS_INSTALL_COMBINED=YES \
    -DCMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE=NO \
    -DCMAKE_MACOSX_BUNDLE=NO \
    -DCMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER="com.gongluck.example" \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO
  cmake --build ./build/apple/$1 --clean-first --target ALL_BUILD --config Release
}

build iOS "arm64;x86_64"
