cmake_minimum_required(VERSION 3.13)
project (ok-ru-capturer)

include_directories($TOOLCHAIN/include)
link_directories($TOOLCHAIN/lib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall -Wextra")

set(LINKING_LIBRARIES "-lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -lcares -lnghttp2")

add_executable (ok-ru-capturer ok-ru-capturer-ng.cpp)
target_link_libraries (ok-ru-capturer ${LINKING_LIBRARIES} -static)