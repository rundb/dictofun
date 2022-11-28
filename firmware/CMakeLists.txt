cmake_minimum_required(VERSION 3.22)

#set(CMAKE_TOOLCHAIN_FILE gcc_arm_none_eabi_10.cmake)

project(Dictofun LANGUAGES C CXX ASM)

include("cmake/nrf5.cmake")

add_subdirectory(src)

