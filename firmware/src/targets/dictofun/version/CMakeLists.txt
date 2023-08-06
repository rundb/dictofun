add_library(version STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

add_custom_command(
    OUTPUT version.cpp

    COMMAND python3 ARGS ${CMAKE_CURRENT_SOURCE_DIR}/version.py ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(version PUBLIC ./)
