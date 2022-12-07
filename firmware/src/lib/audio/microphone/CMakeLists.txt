add_library(microphone_interface INTERFACE)
target_include_directories(microphone_interface INTERFACE ./)
target_link_libraries(microphone_interface INTERFACE
    common
)

if (${is_unit_test})
    add_library(microphone_mock OBJECT
          mock/microphone_mock.cpp  
    )
    target_include_directories(microphone_mock PUBLIC 
        mock/
    )
    target_link_libraries(microphone_mock PUBLIC
        microphone_interface
    )

    add_executable(test_microphone
        test/test_microphone.cpp
    )

    target_link_libraries(test_microphone PRIVATE
        microphone_interface
        microphone_mock
        GTest::gtest_main
    )

    gtest_discover_tests(test_microphone)
else()
    add_library(microphone_pdm INTERFACE)
    target_include_directories(microphone_pdm INTERFACE src)
    target_link_libraries(microphone_pdm INTERFACE microphone_interface)
endif()