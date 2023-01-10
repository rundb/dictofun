add_executable(test_littlefs
    test_littlefs.cpp
    spi_flash_simulation.cpp
)

target_link_libraries(test_littlefs PRIVATE
    GTest::gtest_main
    littlefs
    littlefs_integration
)

gtest_discover_tests(test_littlefs)
