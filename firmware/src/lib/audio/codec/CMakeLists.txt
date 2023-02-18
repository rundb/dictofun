add_library(codec_if INTERFACE)
target_include_directories(codec_if INTERFACE ./)
target_link_libraries(codec_if INTERFACE common)

if (${is_unit_test})
    add_executable(test_codec
        test/test_codec.cpp
    )

    target_link_libraries(test_codec PRIVATE
        codec_if
        GTest::gtest_main
    )

    gtest_discover_tests(test_codec)
else()
    add_library(codec_decimator STATIC
        src/dummy.c
    )

    target_include_directories(codec_decimator PUBLIC src)
    target_link_libraries(codec_decimator PUBLIC 
        codec_if
    )

endif()
