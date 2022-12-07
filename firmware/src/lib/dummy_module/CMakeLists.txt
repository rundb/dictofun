add_library(dummy_module STATIC
    useless_class.cpp
)

target_include_directories(dummy_module PUBLIC ./)

if (${is_unit_test})
    add_executable(test_dummy_module
        test/dummy_test.cpp
    )

    target_link_libraries(test_dummy_module PRIVATE
        dummy_module
        GTest::gtest_main
    )

    gtest_discover_tests(test_dummy_module)
endif()
