add_library(codec_if INTERFACE)
target_include_directories(codec_if INTERFACE ./)
target_link_libraries(codec_if INTERFACE common)

add_library(codec_decimator INTERFACE)
target_include_directories(codec_decimator INTERFACE src/codec_decimate)
target_link_libraries(codec_decimator INTERFACE codec_if)

add_library(codec_adpcm STATIC
    src/codec_adpcm/dvi_adpcm.c
)
target_include_directories(codec_adpcm PUBLIC src/codec_adpcm)
target_link_libraries(codec_adpcm PUBLIC codec_if)

if (${is_unit_test})
    add_executable(test_codec
        test/test_codec.cpp
    )

    target_link_libraries(test_codec PUBLIC
        codec_if
        codec_adpcm
        GTest::gtest_main
    )

    gtest_discover_tests(test_codec)
endif()
