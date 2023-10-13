#!/bin/bash

echo -n "Set toolchain vars... "
export AR=/usr/bin/llvm-ar-17
export AS=/usr/bin/llvm-as-17
export CC=/usr/bin/clang-17
export CXX=/usr/bin/clang++-17
export LD=/usr/bin/llvm-ld-17
export RANLIB=/usr/bin/llvm-ranlib-17
export STRIP=/usr/bin/llvm-strip-17

echo -n "Create build directory... "
mkdir build
cd build

echo -n "Configure ok-ru-capturer... "
cmake -DCMAKE_BUILD_TYPE=Release ..

echo -n "Build ok-ru-capturer... "
make

exit 0
