#!/bin/bash
echo "x64 dependencies builder 1.0.0"

zlibArchive='https://github.com/madler/zlib/releases/download/v1.3/zlib-1.3.tar.xz'
caresArchive='https://github.com/c-ares/c-ares/releases/download/cares-1_20_1/c-ares-1.20.1.tar.gz'
mbedtlsArchive='https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/mbedtls-3.5.0.tar.gz'
nghttp2Archive='https://github.com/nghttp2/nghttp2/releases/download/v1.57.0/nghttp2-1.57.0.tar.xz'
curlArchive='https://github.com/curl/curl/releases/download/curl-8_4_0/curl-8.4.0.tar.xz'

echo -n "Install tools... "
sudo apt update && sudo apt install lsb-release pkg-config wget software-properties-common gnupg git tar xz-utils curl cmake make autoconf libtool -y

echo -n "Install latest Clang... "
mkdir dependencies
cd dependencies
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 17

echo -n "Set vars... "
export AR=/usr/bin/llvm-ar-17
export AS=/usr/bin/llvm-as-17
export CC=/usr/bin/clang-17
export CXX=/usr/bin/clang++-17
export LD=/usr/bin/llvm-ld-17
export RANLIB=/usr/bin/llvm-ranlib-17
export STRIP=/usr/bin/llvm-strip-17

#zlib
echo -n "Download and Extract zlib... "
curl -s -L $zlibArchive | tar --xz -x

echo -n "Configure zlib... "
cd zlib*
./configure --static

echo -n "Build zlib... "
make -j$(nproc)

echo -n "Install zlib... "
sudo make install 
cd ..


#c-ares
echo -n "Download and Extract c-ares... "
curl -s -L $caresArchive | tar zx

echo -n "Configure c-ares... "
cd c-ares*
./configure --disable-shared

echo -n "Build c-ares... "
make -j"$(nproc)" 

echo -n "Install c-ares... "
sudo make install 
cd ..


#mbedtls
echo -n "Download and Extract mbedtls... "
curl -s -L $mbedtlsArchive | tar xz

echo -n "Configure mbedtls... "
cd mbedtls*
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=Off ..
sed -e s/-Wdocumentation//g -i ../library/CMakeLists.txt ../tests/CMakeLists.txt

echo -n "Build mbedtls... "
cmake --build .

echo -n "Install mbedtls... "
sudo make install 
cd ../..


#nghttp2
echo -n "Download and Extract nghttp2... "
curl -s -L $nghttp2Archive | tar --xz -x

echo -n "Configure nghttp2... "
cd nghttp2*
./configure --enable-lib-only --disable-shared

echo -n "Build nghttp2... "
make -j"$(nproc)" 

echo -n "Install nghttp2... "
sudo make install
cd ..


#cURL
echo -n "Download and Extract curl... "
curl -s -L $curlArchive | tar --xz -x

echo -n "Configure curl... "
cd curl*
export LDFLAGS='-lm'
./configure --disable-shared --without-brotli --without-zstd --with-mbedtls=/usr/local --enable-ares=/usr/local --with-nghttp2=/usr/local

echo -n "Build cURL... "
make -j"$(nproc)" 

echo -n "Install cURL... "
sudo make install 

echo -e "\e[32mAll done\e[0m"


exit 0
