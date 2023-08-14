# VCPKG NOTE:
#   configure VCPKG_TARGET_TRIPLET somewhere (either through extension setting
#   or `SET(...)`) prior to the project directive to ensure static libraries
#   are linked (e.g., x64-<platform>-static)

cmake_minimum_required(VERSION 3.27)
project("hello-cube" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/.clang-tidy" "Checks: '-*'")

add_subdirectory("engine")
add_subdirectory("app")
