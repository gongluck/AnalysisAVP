#!/bin/sh

rm -r ./build/linux/*.*
mkdir ./build/linux

cmake -S . -B ./build/linux
#cmake --build ./build/linux --clean-first --config debug --target all
cmake --build ./build/linux --clean-first --config release --target all
