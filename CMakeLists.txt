cmake_minimum_required(VERSION 3.11)
project(kdab-supercede-stdpow-checker)

find_package(Clang REQUIRED)

add_executable(kdab-supercede-stdpow-checker kdab-supercede-stdpow-checker.cpp)
target_link_libraries(kdab-supercede-stdpow-checker clang-cpp LLVM)

install(TARGETS kdab-supercede-stdpow-checker RUNTIME DESTINATION bin)

