add_library(myfs STATIC
    myfs.cpp
    block_api_myfs.cpp
)

target_include_directories(myfs PUBLIC ./
)

target_link_libraries(myfs PUBLIC
    spi_flash_interface
)

target_link_libraries(myfs PRIVATE

)

if (${is_unit_test})
    add_executable(test_myfs
        test/test_myfs.cpp
    )

    target_link_libraries(test_myfs PUBLIC
        myfs
        GTest::gtest_main
    )

    gtest_discover_tests(test_myfs)
endif()
