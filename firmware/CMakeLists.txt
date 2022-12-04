cmake_minimum_required(VERSION 3.22)

# This conditional selection is designed for build selection between dictofun builds and unit test builds.
if ("${BUILD_TARGET}" STREQUAL "dictofun")
    project(Dictofun LANGUAGES C CXX ASM)

    include("cmake/dictofun.cmake")
    include("cmake/nrf5.cmake")

    add_subdirectory(src)
endif()

