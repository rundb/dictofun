cmake_minimum_required(VERSION 3.22)

# This conditional selection is designed for build selection between dictofun builds and unit test builds.
if ("${BUILD_TARGET}" STREQUAL "dictofun")
    project(Dictofun LANGUAGES C CXX ASM)

    set(is_unit_test false)

    include("cmake/dictofun.cmake")
    include("cmake/nrf5.cmake")

    add_subdirectory(src)
elseif("${BUILD_TARGET}" STREQUAL "unit-test")
    project(dictofun_ut LANGUAGES C CXX)
    
    set(CMAKE_CXX_STANDARD 14)

    set(is_unit_test true)

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    add_subdirectory(src/lib/common)
    add_subdirectory(src/lib/dummy_module)
    add_subdirectory(src/lib/audio)
    add_subdirectory(src/lib/spi_flash/interface)
endif()


