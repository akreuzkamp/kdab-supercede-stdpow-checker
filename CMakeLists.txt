cmake_minimum_required(VERSION 3.11)
project(supercedestdpow)

find_package(Clang REQUIRED)

add_executable(supercedestdpow supercedestdpow.cpp)
target_link_libraries(supercedestdpow clang-cpp LLVMCore LLVMSupport)

install(TARGETS supercedestdpow RUNTIME DESTINATION bin)

