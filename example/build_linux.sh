#!/bin/sh

rm -r ./build/linux
mkdir ./build/linux

buildprefix=./build/linux

type=debug
buildpath=$buildprefix/$type
cmake -S . -B $buildpath -DCMAKE_BUILD_TYPE=$type
cmake --build $buildpath --clean-first --config $type --target all -- -j8

type=release
buildpath=$buildprefix/$type
cmake -S . -B $buildpath -DCMAKE_BUILD_TYPE=$type
cmake --build $buildpath --clean-first --config $type --target all -- -j8
