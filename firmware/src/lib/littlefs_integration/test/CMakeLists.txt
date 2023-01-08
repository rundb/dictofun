add_executable(test_littlefs
    test_littlefs.cpp
)

target_link_libraries(test_littlefs PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(test_littlefs)
