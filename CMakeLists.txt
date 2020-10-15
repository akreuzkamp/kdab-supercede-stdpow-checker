cmake_minimum_required(VERSION 3.11)

project(supercedestdpow)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake /usr/lib/cmake/llvm/) # FIXME
find_package(Clang REQUIRED)

include(AddLLVM)
include(AddClang)

include_directories(${CLANG_INCLUDE_DIRS})

set(LLVM_LINK_COMPONENTS support)
add_clang_executable(supercedestdpow supercedestdpow.cpp)

install(TARGETS supercedestdpow RUNTIME DESTINATION bin)

target_link_libraries( supercedestdpow PRIVATE clang-cpp)
